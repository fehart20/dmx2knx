#include <string>
#include <map>

// EIBd (für eibaddr_t)
#include <eibclient.h>

#define N2S(x) #x
#define TOSTRING(x) N2S(x)
#define VERSION __DATE__ " - " __TIME__

#ifndef UTIL_H
#define UTIL_H

typedef uint16_t dmxaddr_t;
typedef uint8_t dmxvalue_t;

/*
	Liest aus Gruppenadressen als const char in den Formaten
	* "<Main group>/<Middle group>/<Subgroup>"
	* "<Main Group>/<Subgroup>"
	* "<Group>
	die eibaddr_t - EIB-Adresse aus.
*/
eibaddr_t getGroupAddr (std::string addr);

/*
	Gibt die Nachricht (msg) aus und beendet das Programm mit Fehlerstatus.
*/
void errorMessage(std::string msg);

/*
	Gibt die Nachricht (msg) aus, das Programm läuft aber weiter
*/
void warningMessage(std::string msg);

/*
	Adresskonfigurationsdatei parsen
	std::map<dmxaddr_t, std::string>, wobei
	dmxaddr_t = dmx Adresse und
	std::string = EIB Group Adresse
*/
std::map<dmxaddr_t, std::string> parseConfigFile(std::string filename);

/*
	Ordner, in dem die C++ - Datei ausgeführt wird erhalten
*/
std::string getBasedir();

#endif
