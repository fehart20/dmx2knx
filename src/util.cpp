#include <algorithm>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cctype>

#include "util.h"

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
