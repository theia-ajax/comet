#pragma once

#include <json.h>

#include "Math2D.h"
#include "StringId.h"

struct json_value_s* JsonLoadFile(const char* FileName);
struct json_object_s* JsonLoadFileAsObject(const char* FileName);
struct json_array_s* JsonLoadFileAsArray(const char* FileName);
bool JsonParseNumber(struct json_value_s* NumberValue, float64* NumberOut);
bool JsonParseBool(struct json_value_s* BoolValue, bool* BoolOut);
bool JsonParseStringId(struct json_value_s* StringValue, StringId* StringIdOut);
bool JsonParseRect(struct json_value_s* RectValue, Rect* RectOut);
bool JsonParseDimensions(struct json_value_s* DimValue, Point* DimOut);
bool JsonParseRect16(struct json_value_s* RectValue, Rect16* RectOut);
bool JsonParseDimensions16(struct json_value_s* DimValue, Point16* DimOut);
bool JsonParseVec2(struct json_value_s* Vec2Value, Vec2* Vec2Out);
bool JsonParseAABB(struct json_value_s* AABBValue, AABB* AABBOut);
struct json_value_s* JsonFindKeyValue(struct json_object_s* Object, const char* Key);
bool JsonGetBool(struct json_object_s* Object, const char* Key, bool Default);
StringId JsonGetStringId(struct json_object_s* Object, const char* Key, StringId Default);
float32 JsonGetFloat32(struct json_object_s* Object, const char* Key, float32 Default);
float64 JsonGetFloat64(struct json_object_s* Object, const char* Key, float64 Default);
int64 JsonGetInt64(struct json_object_s* Object, const char* Key, int64 Default);
int32 JsonGetInt32(struct json_object_s* Object, const char* Key, int32 Default);
Vec2 JsonGetVec2(struct json_object_s* Object, const char* Key, Vec2 Default);
AABB JsonGetAABB(struct json_object_s* Object, const char* Key, AABB Default);
