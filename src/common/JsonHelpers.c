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

bool JsonParseNumber(struct json_value_s* NumberValue, double* NumberOut)
{
	ASSERT(NumberOut);
	*NumberOut = 0.0;

	bool Success = false;
	double Result = 0.0;
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

	struct json_string_s* StringObject = json_value_as_string(StringValue);

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

double JsonGetNumber(struct json_object_s* Object, const char* Key, double Default)
{
	struct json_value_s* NumberValue = JsonFindKeyValue(Object, Key);

	double Result;
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
