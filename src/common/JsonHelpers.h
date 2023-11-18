#pragma once

#include <json.h>

#include "Math2D.h"
#include "StringId.h"

struct json_value_s* JsonLoadFile(const char* fileName);
bool JsonParseNumber(struct json_value_s* NumberValue, double* NumberOut);
bool JsonParseBool(struct json_value_s* BoolValue, bool* BoolOut);
bool JsonParseStringId(struct json_value_s* StringValue, StringId* StringIdOut);
bool JsonParseRect(struct json_value_s* RectValue, Rect* RectOut);
bool JsonParseDimensions(struct json_value_s* DimValue, Point* DimOut);
bool JsonParseRect16(struct json_value_s* RectValue, Rect16* RectOut);
bool JsonParseDimensions16(struct json_value_s* DimValue, Point16* DimOut);
struct json_value_s* JsonFindKeyValue(struct json_object_s* Object, const char* Key);
bool JsonGetBool(struct json_object_s* Object, const char* Key, bool Default);
StringId JsonGetStringId(struct json_object_s* Object, const char* Key, StringId Default);
double JsonGetNumber(struct json_object_s* Object, const char* Key, double Default);
int64 JsonGetInt64(struct json_object_s* Object, const char* Key, int64 Default);
int32 JsonGetInt32(struct json_object_s* Object, const char* Key, int32 Default);
