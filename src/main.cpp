// *** KONFIGURATION ****
// URL, unter der EIBd erreichbar ist
#define EIBD_URL "ip:127.0.0.1"

// Datei, in der die DMX-EIB-Adresszuordnungen liegen
#define CONFIG_FILENAME "pairs.conf"

// Timeout beim auslesen der DMX-Daten aus dem FX5-Interface (Millisekunden)
#define FX5_READ_TIMEOUT 100

// Die Ausgabewerte werden schrittweise ausgegeben. Zeit zwischen zwei Ausgaben in
// Millisekunden.
#define EIB_WRITE_DELAY 50

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

// EIBd
#include <eibclient.h>
#include "util.h"

// ########################################################
// ##                   KNX/EIB Output                   ##
// ########################################################
EIBConnection *eibd = nullptr;

enum EIB_fixture_type
{
	EIB_VALUE = 1,
	EIB_SWITCH = 2
};

class EIB_fixture
{
	public:
		EIB_fixture(eibaddr_t addr, EIB_fixture_type type) :
			m_addr(addr),
			m_type(type),
			m_init(false),
			m_lastval(0),
			m_new_value(false) {};

		void onDMXInput(dmxvalue_t val)
		{
			m_mutex.lock();
			// Erster Wert: ignorieren, und als gegeben annehmen
			if (!m_init)
			{
				m_lastval = val;
				m_init = true;
				m_mutex.unlock();
				return;
			}

			// Bei Änderungen des Wertes diese ausgeben
			if (val != m_lastval)
			{
				m_lastval = val;
				m_new_value = true;
			}
			m_mutex.unlock();
		}

		// Gibt true zurück, wenn ein neuer EIB-Wert ausgegeben wurde
		// Gibt false zurück, wenn kein neuer Wert vorhanden ist
		// Nach "KNX BASIC COURSE", Seite 16
		// http://www.knx.org/media/docs/KNX-Tutor-files/Summary/KNX-Communication.pdf
		bool updateOutput()
		{
			m_mutex.lock();
			if (!m_new_value)
			{
				m_mutex.unlock();
				return false;
			}

			uint8_t buffer[3] = {0x00};
			uint8_t msg_len = 0;
			if (m_type == EIB_SWITCH) // 1-bit
			{
				msg_len = 2;
				if (m_lastval > 127)
					buffer[1] = 0x81; // on
				else
					buffer[1] = 0x80; // off
				std::cout << "[OUTPUT] KNX=" << m_addr << " -> " <<
					(m_lastval > 127 ? "on" : "off") << std::endl;
			}
			else if (m_type == EIB_VALUE) // 1 byte
			{
				msg_len = 3;
				buffer[1] = 0x80;
				buffer[2] = m_lastval;
				std::cout << "[OUTPUT] KNX=" << m_addr << " -> " <<
					(m_lastval / 2.55) << "%" << std::endl;
			}
			m_new_value = false;

			if (EIBOpenT_Group (eibd, m_addr, 1) == -1)
				errorMessage("Gruppe nicht gefunden");

			EIBSendAPDU (eibd, msg_len, buffer);

			m_mutex.unlock();
			return true;
		}

	private:
		// Die EIB-Gruppenadresse dieses Gateways
		eibaddr_t m_addr;

		// Der Gruppentyp (Schalter oder Wert = Dimmen)
		EIB_fixture_type m_type;

		// Speichert, ob schon ein gültiger Wert eingelesen wurde
		bool m_init;

		// Speichert den letzten DMX-Eingabewert
		dmxvalue_t m_lastval;

		// Speichert, ob ein Ausgabe-Update erforderlich ist
		bool m_new_value;

		// Kontrolliert, dass der EIB-Thread und der DMX-Thread nicht gleichzeitig
		// auf variablen zugreifen.
		std::mutex m_mutex;
};

std::map<dmxaddr_t, EIB_fixture*> gateways;

void loadConfig(std::string filename)
{
	std::cout << "Gateway-Verbindungen:" << std::endl;
	std::cout << "-------------------------------------" << std::endl;

	// Datei mit filename öffnen
	std::ifstream file;
	file.open(getBasedir() + filename.c_str());

	// Zeile für Zeile parsen
	std::string line;
	while (std::getline(file, line))
	{
		// Leere Linien ignorieren
		if (line.length() == 0) continue;

		// Leerzeichen & Tabs entfernen
		line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

		// Linien, die mit # beginnen ignorieren
		if (line.at(0) == '#') continue;

		// Position des ersten Gleichheitszeichens (=)
		size_t eq1_pos = line.find_first_of("=");

		// Position des zweiten Gleichheitszeichens (=)
		size_t eq2_pos = line.find("=", eq1_pos + 1);

		// Wert links vom ersten = (DMX Adresse)
		std::string dmxaddr_str = line.substr(0, eq1_pos);

		// Wert rechts vom ersten = (EIB Adresse) bis zum zweiten =
		std::string eibaddr_str = line.substr(eq1_pos + 1, eq2_pos - eq1_pos - 1);

		// Wert rechts vom zweiten = (EIB Typ)
		std::string fixtype_str = line.substr(eq2_pos + 1, line.length() - eq2_pos);

		// DMX Adresse von std::string zu dmxaddr_t
		dmxaddr_t dmxaddr;
		std::istringstream convert(dmxaddr_str);
		if (!(convert >> dmxaddr))
			errorMessage("Fehlerhafte DMX Adresse in Config (" + dmxaddr_str + ")");

		// EIB Adresse von std::string zu eibaddr_t
		eibaddr_t eibaddr = getGroupAddr(eibaddr_str);
		if (eibaddr == 0x00)
			errorMessage("Ungültiges Format für Gruppenadresse in Config: " +
				std::string(eibaddr_str));

		// EIB Adresstyp von std::string zu EIB_fixture_type
		EIB_fixture_type fixtype;
		if (fixtype_str == "SWITCH")
			fixtype = EIB_SWITCH;
		else if (fixtype_str == "VALUE")
			fixtype = EIB_VALUE;
		else
			errorMessage("Ungültiger EIB-Typ: " + fixtype_str);

		// DMX <--> EIB Adresspaar als GateWay registrieren
		gateways[dmxaddr] = new EIB_fixture(eibaddr, fixtype);
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
		// Alle EIB_WRITE_DELAY Millisekunden Werte ausgeben
		std::chrono::milliseconds delay(EIB_WRITE_DELAY);
		std::this_thread::sleep_for(delay);

		// Niedrigste DMX-Adressen werden zuerst verarbeitet
		for (auto g : gateways)
			if (g.second->updateOutput()) break;
	}
}


// ########################################################
// ##                      DMX Input                     ##
// ########################################################
void onDMXInput(dmxaddr_t addr, dmxvalue_t val)
{
	// Wenn Gateway registriert ist, EIB-Signal ausgeben
	if (gateways.find(addr) != gateways.end())
	{
		std::cout << "[INPUT ] DMX=" << addr << " -> " << std::to_string(val) << std::endl;
		gateways[addr]->onDMXInput(val);
	}
}

int main()
{
	std::cout << "DMX-EIB-Gateway startet. Build: " << VERSION << std::endl;

	// Config-Datei laden und ausgeben
	loadConfig(CONFIG_FILENAME);

	// EIB initialisieren
	eibd = EIBSocketURL(EIBD_URL);
	EIBOpen_GroupSocket (eibd, 1);
	if (!eibd) errorMessage("Konnte keine Verbindung zum eibd-Daemon aufbauen");
	// EIB-Output-Thread starten
	std::thread t(eibOutputThread);
	t.detach();

	// DMX FX5 initialisieren (Vendor ID + Product ID: Digital Enlightenment sunlight killer)
	hid_device *fx5;
	fx5 = hid_open(0xb404, 0x1f0f, NULL);
	if (!fx5)
		errorMessage("Konte USB-DMX-Interface nicht finden");

	// FX5 als input verwenden
	uint8_t cmd[34];
	memset(cmd, 0, 34);
	cmd[1] = 16;
	cmd[2] = 4; // (4 = input only mode)
	hid_write(fx5, cmd, 34);

	// DMX input polling
	for (;;)
	{
		// DMX-Input lesen
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
			if (size == 33)
				for (int i = 0; i < 32; i++)
					onDMXInput(buffer[0] * 32 + i, buffer[i + 1]);
	}
}
