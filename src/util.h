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
	Gibt die Nachricht (msg) aus und beendet das Programm mit Fehlerstatus.
*/
void errorMessage(std::string msg);

/*
	Gibt die Nachricht (msg) aus, das Programm läuft aber weiter
*/
void warningMessage(std::string msg);

/*
	Ordner, in dem die C++ - Datei ausgeführt wird erhalten
*/
std::string getBasedir();

#endif
