#pragma once

#include "ini.h"
#include "StringId.h"
#include "Types.h"

ini_t* IniLoadFile(const char *FileName);
bool IniHasSection(ini_t* Ini, const char* Section);
bool IniHasProperty(ini_t* Ini, int Section, const char* Property);
const char* IniReadString(ini_t* Ini, int Section, const char* Property, const char* Default);
StringId IniReadStringId(ini_t* Ini, int Section, const char* Property, StringId Default);
int IniReadInt(ini_t* Ini, int Section, const char* Property, int Default);
double IniReadFloat(ini_t* Ini, int Section, const char* Property, double Default);
bool IniReadBool(ini_t* Ini, int Section, const char* Property, bool Default);
