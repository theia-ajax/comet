#define INI_IMPLEMENTATION
#include "ini.h"

#include "IniHelpers.h"

ini_t* IniLoadFile(const char *FileName)
{
	
}

bool IniHasSection(ini_t* Ini, const char* Section)
{
	return ini_find_section(Ini, Section, 0) != INI_NOT_FOUND;
}

bool IniHasProperty(ini_t* Ini, int Section, const char* Property)
{
	return ini_find_property(Ini, Section, Property, 0) != INI_NOT_FOUND;
}

const char* IniReadString(ini_t* Ini, int Section, const char* Property, const char* Default)
{
	const char* Result = Default;
	int PropertyIndex = ini_find_property(Ini, Section, Property, 0);
	if (PropertyIndex != INI_NOT_FOUND) {
		Result = ini_property_value(Ini, Section, PropertyIndex);
	}
	return Result;
}

StringId IniReadStringId(ini_t* Ini, int Section, const char* Property, StringId Default)
{
	StringId Result = KInvalidStringId;
	const char* StringValue = IniReadString(Ini, Section, Property, NULL);
	if (StringValue) {
		Result = GetStringId(StringValue);
	}
	return Result;
}

int IniReadInt(ini_t* Ini, int Section, const char* Property, int Default)
{
	int Result = Default;

	const char* Value = IniReadString(Ini, Section, Property, NULL);
	if (Value) {
		Result = strtol(Value, NULL, 10);
	}
	return Result;
}

double IniReadFloat(ini_t* Ini, int Section, const char* Property, double Default)
{
	double Result = Default;

	const char* Value = IniReadString(Ini, Section, Property, NULL);
	if (Value) {
		Result = strtod(Value, NULL);
	}
	return Result;
}

bool IniReadBool(ini_t* Ini, int Section, const char* Property, bool Default)
{
	bool Result = Default;

	const char* Value = IniReadString(Ini, Section, Property, NULL);
	if (Value) {
		Result = Value[0] == 't' || Value[0] == 'T';
	}
	return Result;
}

