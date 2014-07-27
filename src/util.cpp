#include <algorithm>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cctype>

#include "util.h"

/*
	Aus BCU SDK Version 0.0.5, /bcusdk-0.0.5/eibd/examples/common.c
*/

eibaddr_t getGroupAddr (std::string addr)
{
	const char *addr_c = addr.c_str();
	int a, b, c;

	if (sscanf (addr_c, "%d/%d/%d", &a, &b, &c) == 3)
		return ((a & 0x01f) << 11) | ((b & 0x07) << 8) | ((c & 0xff));
	if (sscanf (addr_c, "%d/%d", &a, &b) == 2)
		return ((a & 0x01f) << 11) | ((b & 0x7FF));
	if (sscanf (addr_c, "%x", &a) == 1)
		return a & 0xffff;

	return 0x00;
}

void errorMessage(std::string msg)
{
	std::cerr << "Fehler: " << msg << "! Programm wird beendet." << std::endl;
	std::exit(EXIT_FAILURE);
}

void warningMessage(std::string msg)
{
	std::cerr << "Warnung: " << msg << "! Programm läuft weiter." << std::endl;
}

#define MAX_PATHLEN 512
std::string getBasedir()
{
	char buf[MAX_PATHLEN];
	ssize_t len = readlink("/proc/self/exe", buf, MAX_PATHLEN-1);
	buf[len] = 0x00;

	std::string path = std::string(buf);		// jetzt in saallicht/<executable>
	path = path.substr(0, path.find_last_of("/"));	// jetzt in saallicht
	
	return path + "/";
}
