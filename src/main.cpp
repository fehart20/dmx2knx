// *** CONFIGURATION ****
// URL to be used for connnection to EIBd
#define EIBD_URL "ip:127.0.0.1"

// DMX-EIB address assignments file
#define CONFIG_FILENAME "pairs.conf"

// Timeout for reading the DMX data from the Digital Enlightenment USB-DMX interface (in milliseconds)
#define FX5_READ_TIMEOUT 100

// Changed EIB data values are sent on a regular basis. Delay between sending two EIB values.
#define EIB_WRITE_DELAY 10

// Ignore first input value of DMX interface, disabled by default.
#define IGNORE_FIRST_VALUE false

// C++ includes
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cstring>
#include <string>
#include <thread>
#include <mutex>
#include <map>

// HIDapi for FX5
#include <hidapi/hidapi.h>
#include "util.h"

// ########################################################
// ##                   KNX/EIB Output                   ##
// ########################################################
enum EIB_fixture_type
{
	EIB_VALUE = 1,
	EIB_SWITCH = 2
};

class EIB_fixture
{
	public:
		EIB_fixture(std::string addr, EIB_fixture_type type) :
			m_addr(addr),
			m_type(type),
			m_init(!IGNORE_FIRST_VALUE),
			m_lastval(0),
			m_new_value(false) {};

		// Returns true if given DMX value has changed
		bool onDMXInput(dmxvalue_t val)
		{
			// Ignore first DMX input value and set m_lastval
			if (!m_init)
			{
				m_lastval = val;
				m_init = true;
				return false;
			}

			// Output DMX value changes to EIB bus
			if (val != m_lastval)
			{
				m_lastval = val;
				m_new_value = true;
				return true;
			}

			return false;
		}

		// Returns true if a new EIB values is available and outputs it
		// Returns false if no new EIB value is available
		// Simply calls groupswrite (switch) or groupwrite (value)
		bool updateOutput()
		{
			if (!m_new_value) return false;

			if (m_type == EIB_SWITCH) // 1-bit
			{
				system(("groupswrite " + std::string(EIBD_URL) + " " + m_addr + " "
					+ ((m_lastval > 127) ? "1" : "0")).c_str());
				std::cout << "[OUTPUT] KNX=" << m_addr << " -> " <<
					(m_lastval > 127 ? "on" : "off") << std::endl;
			}
			else if (m_type == EIB_VALUE) // 1 byte
			{
				int8_t output = m_lastval / 2.55 - 1;
				if (output < 0) output = 0;
				system(("groupwrite " + std::string(EIBD_URL) + " " + m_addr + " "
					+ std::to_string(output)).c_str());
				std::cout << "[OUTPUT] KNX=" << m_addr << " -> " <<
					std::to_string(output) << std::endl;
			}
			m_new_value = false;

			return true;
		}

	private:
		// EIB group address (as string) for the connected EIB fixture
		std::string m_addr;

		// EIB fixture type, either switch (1bit, on/off) or value (dimming 0-100%)
		EIB_fixture_type m_type;

		// Saves if a valid value has been input from the DMX interface yet
		bool m_init;

		// Saves last DMX input value
		dmxvalue_t m_lastval;

		// True, if a new value is available to be output through EIBd
		bool m_new_value;
};

std::map<dmxaddr_t, EIB_fixture*> gateways;

void loadConfig(std::string filename)
{
	std::cout << "Gateway-Verbindungen:" << std::endl;
	std::cout << "-------------------------------------" << std::endl;

	// Open file with given filename
	std::ifstream file;
	file.open(getBasedir() + filename.c_str());

	// Parse config line by line
	std::string line;
	while (std::getline(file, line))
	{
		// Ignore empty lines
		if (line.length() == 0) continue;

		// Remove tabs & spaces
		line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

		// Ignore lines starting with # (=comments)
		if (line.at(0) == '#') continue;

		// Position of first equals sign (=)
		size_t eq1_pos = line.find_first_of("=");

		// Position of second equals sign (=)
		size_t eq2_pos = line.find("=", eq1_pos + 1);

		// Read value left of the first equals sign (DMX address)
		std::string dmxaddr_str = line.substr(0, eq1_pos);

		// Read value between first and second equals sign (EIB address)
		std::string eibaddr_str = line.substr(eq1_pos + 1, eq2_pos - eq1_pos - 1);

		// Read value right of the second equals sign (Type: Switch or value)
		std::string fixtype_str = line.substr(eq2_pos + 1, line.length() - eq2_pos);

		// Convert DMX address from std::string to dmxaddr_t
		dmxaddr_t dmxaddr;
		std::istringstream convert(dmxaddr_str);
		if (!(convert >> dmxaddr))
			errorMessage("Fehlerhafte DMX Adresse in Config (" + dmxaddr_str + ")");

		// Convert EIB fixture type from std::string to EIB_fixture_type (EIB_SWITCH or EIB_VALUE)
		EIB_fixture_type fixtype;
		if (fixtype_str == "SWITCH")
			fixtype = EIB_SWITCH;
		else if (fixtype_str == "VALUE")
			fixtype = EIB_VALUE;
		else
			errorMessage("Ungültiger EIB-Typ: " + fixtype_str);

		// Register DMX <--> EIB address pair as gateway
		gateways[dmxaddr] = new EIB_fixture(eibaddr_str, fixtype);
		std::cout << "DMX: " << dmxaddr << " --> EIB: " << eibaddr_str;
		std::cout << " als " << (fixtype == EIB_SWITCH ? "Schalter" : "Wert") << std::endl;
	}

	std::cout << "-------------------------------------" << std::endl;
}

// KNX/EIB Output thread
void eibOutputThread()
{
	for(;;)
	{
		// Output new EIB values every EIB_WRITE_DELAY milliseconds
		std::chrono::milliseconds delay(EIB_WRITE_DELAY);
		std::this_thread::sleep_for(delay);

		// Only output one value. Lower DMX addresses are prioritized
		for (auto g : gateways)
			if (g.second->updateOutput()) break;
	}
}


// ########################################################
// ##                      DMX Input                     ##
// ########################################################
void onDMXInput(dmxaddr_t addr, dmxvalue_t val)
{
	// Call onDMXInput on gateway if registered
	if (gateways.find(addr) != gateways.end())
		if (gateways[addr]->onDMXInput(val))
			std::cout << "[INPUT ] DMX=" << addr << " -> " << std::to_string(val) << std::endl;
}

int main()
{
	std::cout << "DMX-EIB-Gateway startet. Build: " << VERSION << std::endl;

	// Parse config file, register gateways and output configuration
	loadConfig(CONFIG_FILENAME);

	// Initialize USB-DMX interface (Vendor ID + Product ID: Digital Enlightenment Sunlight Killer)
	// !!! If you're using the FX5 interface instead of the Digital Enlightenment Sunlight Killer you have to change these lines !!!
	hid_device *fx5;
	fx5 = hid_open(0x04b4, 0x0f1f, NULL);
	if (!fx5)
		errorMessage("Konte USB-DMX-Interface nicht finden");

	// Use FX5 as output
	uint8_t cmd[34];
	memset(cmd, 0, 34);
	cmd[1] = 16;
	cmd[2] = 4; // (4 = input only mode)
	hid_write(fx5, cmd, 34);

	// Create and detach EIB output thread
	std::thread t(eibOutputThread);
	t.detach();

	// DMX input polling
	for (;;)
	{
		// Read DMX input
		uint8_t buffer[35];
		int size;
		size = hid_read_timeout(fx5, buffer, 33, FX5_READ_TIMEOUT);

		/**
		 * Protocol: 33 bytes in buffer[33]
		 * [0]      = chunk, which is the offset by which the channel is calculated
		 *            from, the nth chunk starts at address n * 32
		 * [1]-[32] = channel values, where the nth value is the offset + n
		*/
		while (size > 0)
		{
			if (size == 33)
				for (int i = 0; i < 32; i++)
					onDMXInput(buffer[0] * 32 + i + 1, buffer[i + 1]);
			size = hid_read_timeout(fx5, buffer, 33, FX5_READ_TIMEOUT);
		}
	}
}
