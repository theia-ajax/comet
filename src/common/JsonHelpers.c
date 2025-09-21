#include "JsonHelpers.h"

#include <SDL3/SDL.h>

struct json_value_s* JsonLoadFile(const char* fileName)
{
	size_t Size;
	void* FileData = SDL_LoadFile(fileName, &Size);

	if (FileData == NULL) {
		return NULL;
	}

	struct json_value_s* ParsedJson = json_parse(FileData, Size);

	SDL_free(FileData);

	return ParsedJson;
}

struct json_object_s* JsonLoadFileAsObject(const char* FileName)
{
	return json_value_as_object(JsonLoadFile(FileName));
}

struct json_array_s* JsonLoadFileAsArray(const char* FileName)
{
	return json_value_as_array(JsonLoadFile(FileName));
}

bool JsonParseNumber(struct json_value_s* NumberValue, float64* NumberOut)
{
	ASSERT(NumberOut);
	*NumberOut = 0.0;

	bool Success = false;
	float64 Result = 0.0;
	struct json_number_s* NumberObject = json_value_as_number(NumberValue);
	if (NumberObject != NULL) {
		Result = strtod(NumberObject->number, NULL);
		Success = true;
	} else {
		struct json_string_s* StringObject = json_value_as_string(NumberValue);
		if (StringObject != NULL) {
			Result = strtod(StringObject->string, NULL);
			Success = true;
		}
	}
	*NumberOut = Result;
	return Success;
}

bool JsonParseBool(struct json_value_s* BoolValue, bool* BoolOut)
{
	ASSERT(BoolOut);
	*BoolOut = false;

	if (BoolValue == NULL) {
		return false;
	}

	*BoolOut = BoolValue->type == json_type_true;
	return (BoolValue->type == json_type_true || BoolValue->type == json_type_false);
}

bool JsonParseStringId(struct json_value_s* StringValue, StringId* StringIdOut)
{
	ASSERT(StringIdOut);
	ZERO_STRUCT(StringIdOut);

	struct json_string_s* StringObject = (StringValue) ? json_value_as_string(StringValue) : NULL;

	if (StringObject == NULL) {
		return false;
	}

	*StringIdOut = GetStringIdN(StringObject->string, StringObject->string_size);

	return true;
}

bool JsonParseRect(struct json_value_s* RectValue, Rect* RectOut)
{
	ASSERT(RectOut);
	ZERO_STRUCT(RectOut);

	struct json_object_s* RectObject = json_value_as_object(RectValue);

	if (RectObject == NULL || RectObject->length != 4) {
		return false;
	}

	RectOut->X = JsonGetInt32(RectObject, "x", 0);
	RectOut->Y = JsonGetInt32(RectObject, "y", 0);
	RectOut->W = JsonGetInt32(RectObject, "w", 0);
	RectOut->H = JsonGetInt32(RectObject, "h", 0);

	return true;
}

bool JsonParseDimensions(struct json_value_s* DimValue, Point* DimOut)
{
	ASSERT(DimOut);
	ZERO_STRUCT(DimOut);

	struct json_object_s* DimObject = json_value_as_object(DimValue);

	if (DimObject == NULL || DimObject->length != 2) {
		return false;
	}

	DimOut->X = JsonGetInt32(DimObject, "w", 0);
	DimOut->Y = JsonGetInt32(DimObject, "h", 0);

	return true;
}

bool JsonParseRect16(struct json_value_s* RectValue, Rect16* RectOut)
{
	ASSERT(RectOut);
	ZERO_STRUCT(RectOut);

	struct json_object_s* RectObject = json_value_as_object(RectValue);

	if (RectObject == NULL || RectObject->length != 4) {
		return false;
	}

	RectOut->X = (int16)JsonGetInt32(RectObject, "x", 0);
	RectOut->Y = (int16)JsonGetInt32(RectObject, "y", 0);
	RectOut->W = (int16)JsonGetInt32(RectObject, "w", 0);
	RectOut->H = (int16)JsonGetInt32(RectObject, "h", 0);

	return true;
}

bool JsonParseDimensions16(struct json_value_s* DimValue, Point16* DimOut)
{
	ASSERT(DimOut);
	ZERO_STRUCT(DimOut);

	struct json_object_s* DimObject = json_value_as_object(DimValue);

	if (DimObject == NULL || DimObject->length != 2) {
		return false;
	}

	DimOut->X = (int16)JsonGetInt32(DimObject, "w", 0);
	DimOut->Y = (int16)JsonGetInt32(DimObject, "h", 0);

	return true;
}

bool JsonParseVec2(struct json_value_s* Vec2Value, Vec2* Vec2Out)
{
	ASSERT(Vec2Out != NULL);
	ZERO_STRUCT(Vec2Out);

	struct json_object_s* Vec2Object = json_value_as_object(Vec2Value);

	if (Vec2Object == NULL) {
		return false;
	}

	Vec2Out->X = JsonGetFloat64(Vec2Object, "x", 0.0);
	Vec2Out->Y = JsonGetFloat64(Vec2Object, "y", 0.0);

	return true;
}

bool JsonParseAABB(struct json_value_s* AABBValue, AABB* AABBOut)
{
	ASSERT(AABBOut != NULL);
	ZERO_STRUCT(AABBOut);

	struct json_object_s* AABBObject = json_value_as_object(AABBValue);

	if (AABBObject == NULL) {
		return false;
	}

	AABBOut->MinBound = JsonGetVec2(AABBObject, "min_bound", V2(0, 0));
	AABBOut->MaxBound = JsonGetVec2(AABBObject, "max_bound", V2(0, 0));
	return true;
}

struct json_value_s* JsonFindKeyValue(struct json_object_s* Object, const char* Key)
{
	if (Object == NULL) {
		return NULL;
	}

	struct json_object_element_s* Current = Object->start;
	while (Current != NULL) {
		if (strcmp(Current->name->string, Key) == 0) {
			return Current->value;
		}
		Current = Current->next;
	}

	return NULL;
}

bool JsonGetBool(struct json_object_s* Object, const char* Key, bool Default)
{
	struct json_value_s* BoolValue = JsonFindKeyValue(Object, Key);

	bool Result;
	if (JsonParseBool(BoolValue, &Result)) {
		return Result;
	}
	return Default;
}

StringId JsonGetStringId(struct json_object_s* Object, const char* Key, StringId Default)
{
	struct json_value_s* StringValue = JsonFindKeyValue(Object, Key);

	StringId Result;
	if (JsonParseStringId(StringValue, &Result)) {
		return Result;
	}
	return Default;
}

float32 JsonGetFloat32(struct json_object_s* Object, const char* Key, float32 Default)
{
	struct json_value_s* NumberValue = JsonFindKeyValue(Object, Key);

	float64 Result;
	if (JsonParseNumber(NumberValue, &Result)) {
		return (float32)Result;
	}
	return Default;
}

float64 JsonGetFloat64(struct json_object_s* Object, const char* Key, float64 Default)
{
	struct json_value_s* NumberValue = JsonFindKeyValue(Object, Key);

	float64 Result;
	if (JsonParseNumber(NumberValue, &Result)) {
		return Result;
	}
	return Default;
}

int64 JsonGetInt64(struct json_object_s* Object, const char* Key, int64 Default)
{
	struct json_value_s* NumberValue = JsonFindKeyValue(Object, Key);

	double Result;
	if (JsonParseNumber(NumberValue, &Result)) {
		return (int64)Result;
	}
	return Default;
}

int32 JsonGetInt32(struct json_object_s* Object, const char* Key, int32 Default)
{
	struct json_value_s* NumberValue = JsonFindKeyValue(Object, Key);

	double Result;
	if (JsonParseNumber(NumberValue, &Result)) {
		return (int32)Result;
	}
	return Default;
}

Vec2 JsonGetVec2(struct json_object_s* Object, const char* Key, Vec2 Default)
{
	struct json_value_s* Vec2Value = JsonFindKeyValue(Object, Key);

	Vec2 Result;
	if (JsonParseVec2(Vec2Value, &Result)) {
		return Result;
	}
	return Default;
}

AABB JsonGetAABB(struct json_object_s* Object, const char* Key, AABB Default)
{
	struct json_value_s* AABBValue = JsonFindKeyValue(Object, Key);

	AABB Result;
	if (JsonParseAABB(AABBValue, &Result)) {
		return Result;
	}
	return Default;
}