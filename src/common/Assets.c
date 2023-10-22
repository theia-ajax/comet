#include "Assets.h"

#include <SDL2/SDL.h>
#include <json.h>
#include <stb_ds.h>
#include <stb_image.h>

#include "util.h"

// Private Definitions

typedef int32 FreeStack;

struct {
	AssetsConfig Config;
	ImageAsset* ImageAssets;
	FreeStack* ImageAssetsFreeStack;
	SpriteSheetAsset* SpriteSheetAssets;
} GAssets;

// Private Prototypes

static StringId JsonStringId(const struct json_string_s* jsonString);
static bool JsonKeyEq(const struct json_object_element_s* element, const char* key);

static ImageAsset* AllocateImageAsset(void);
static void ReleaseImageAsset(ImageAsset* image);
static void FreeImageAssetResources(ImageAsset* image);

static SpriteSheetAsset* AllocateSpriteSheetAsset(void);
static void _ReleaseLastAllocatedSpriteSheetAsset(void);

static bool ParseSpriteSheetMetaData(
	struct json_value_s* MetaObjectValue,
	SpriteSheetMetaData* DataOut);
static bool ParseSpriteSheetFrameData(
	struct json_value_s* FrameArrayValue,
	SpriteSheetFramesData* DataOut);
static bool ParseSpriteFrameData(
	struct json_value_s* FrameValue,
	SpriteSheetFramesData* DataOut,
	int32 Id);
static bool ParseNumber(struct json_value_s* NumberValue, double* NumberOut);
static bool ParseBool(struct json_value_s* BoolValue, bool* BoolOut);
static bool ParseStringId(struct json_value_s* StringValue, StringId* StringIdOut);
static bool ParseRect(struct json_value_s* RectValue, Rect* RectOut);
static bool ParseDimensions(struct json_value_s* DimValue, Point* DimOut);
static struct json_value_s* JsonFindKeyValue(struct json_object_s* Object, const char* Key);
static bool JsonGetBool(struct json_object_s* Object, const char* Key, bool Default);
static StringId JsonGetStringId(struct json_object_s* Object, const char* Key, StringId Default);
static double JsonGetNumber(struct json_object_s* Object, const char* Key, double Default);
static int64 JsonGetInt64(struct json_object_s* Object, const char* Key, int64 Default);
static int32 JsonGetInt32(struct json_object_s* Object, const char* Key, int32 Default);

// Public Implementations

void AssetsInitialize(const AssetsConfig* config)
{
	arrsetcap(GAssets.ImageAssets, 256);
	arrsetcap(GAssets.ImageAssetsFreeStack, 256);

	arrsetcap(GAssets.SpriteSheetAssets, 16);
}

void AssetsShutdown(void)
{
	for (size_t ImageIndex = 0; ImageIndex < arrlenu(GAssets.ImageAssets); ImageIndex++) {
		FreeImageAssetResources(&GAssets.ImageAssets[ImageIndex]);
	}

	arrfree(GAssets.ImageAssets);
	arrfree(GAssets.ImageAssetsFreeStack);

	arrfree(GAssets.SpriteSheetAssets);
}

ImageAsset* LoadImageAsset(const char* fileName)
{
	ImageAsset* Result = AllocateImageAsset();

	// Hardcoding 4 bytes per pixel regardless of source because lazy

	int Width, Height, Channels;
	Result->Pixels = stbi_load(fileName, &Width, &Height, &Channels, 4);

	if (Result->Pixels == NULL) {
		ReleaseImageAsset(Result);
		return NULL;
	}

	Result->Width = (int32)Width;
	Result->Height = (int32)Height;
	Result->Pitch = Result->Width * 4;

	Result->Surface = SDL_CreateRGBSurfaceWithFormatFrom(
		Result->Pixels, Result->Width, Result->Height, 32, Result->Pitch, SDL_PIXELFORMAT_ABGR8888);

	return Result;
}

void UnloadImageAsset(ImageAsset* image)
{
	FreeImageAssetResources(image);
	ReleaseImageAsset(image);
}

SpriteSheetAsset* LoadSpriteSheetAsset(const char* fileName)
{
	size_t Size;
	void* FileData = SDL_LoadFile(fileName, &Size);

	if (FileData == NULL) {
		return NULL;
	}

	SpriteSheetAsset* Result = NULL;

	struct json_value_s* ParsedJson = json_parse(FileData, Size);
	struct json_object_s* Object = json_value_as_object(ParsedJson);

	if (Object == NULL || Object->length != 2) {
		goto CleanUp;
	}

	Result = AllocateSpriteSheetAsset();

	bool MetaObjectParsed = false, FramesArrayParsed = false;
	struct json_array_s* FramesArray = NULL;
	struct json_object_s* MetaObject = NULL;

	struct json_object_element_s* Current = Object->start;
	while (Current) {
		if (JsonKeyEq(Current, "meta")) {
			MetaObjectParsed = ParseSpriteSheetMetaData(Current->value, &Result->Meta);
		}
		if (JsonKeyEq(Current, "frames")) {
			FramesArrayParsed = ParseSpriteSheetFrameData(Current->value, &Result->Frames);
		}
		Current = Current->next;
	}

	if (!(MetaObjectParsed && FramesArrayParsed)) {
		_ReleaseLastAllocatedSpriteSheetAsset();
	}

CleanUp:
	SDL_free(FileData);
	return Result;
}

// Private Implementations

static StringId JsonStringId(const struct json_string_s* jsonString)
{
	return GetStringIdN(jsonString->string, jsonString->string_size);
}

static bool JsonKeyEq(const struct json_object_element_s* element, const char* key)
{
	bool Result = strcmp(element->name->string, key) == 0;
	return Result;
}

static ImageAsset* AllocateImageAsset(void)
{
	if (arrlen(GAssets.ImageAssetsFreeStack) > 0) {
		int32 Index = arrpop(GAssets.ImageAssetsFreeStack);
		return &GAssets.ImageAssets[Index];
	}

	arrput(GAssets.ImageAssets, (ImageAsset){0});
	return arrlastp(GAssets.ImageAssets);
}

static void ReleaseImageAsset(ImageAsset* image)
{
	ASSERT(image != NULL);

	ptrdiff_t IndexOf = image - GAssets.ImageAssets;
	ASSERT(VALID_INDEX(IndexOf, arrlen(GAssets.ImageAssets)));

	arrput(GAssets.ImageAssetsFreeStack, IndexOf);
}

static void FreeImageAssetResources(ImageAsset* image)
{
	SDL_FreeSurface(image->Surface);
	stbi_image_free(image->Pixels);
	ZERO_STRUCT(image);
}

static SpriteSheetAsset* AllocateSpriteSheetAsset(void)
{
	arrput(GAssets.SpriteSheetAssets, (SpriteSheetAsset){0});
	return arrlastp(GAssets.SpriteSheetAssets);
}

static void _ReleaseLastAllocatedSpriteSheetAsset(void)
{
	arrpop(GAssets.SpriteSheetAssets);
}

static bool ParseSpriteSheetMetaData(
	struct json_value_s* MetaObjectValue,
	SpriteSheetMetaData* DataOut)
{
	ASSERT(DataOut);
	ZERO_STRUCT(DataOut);

	struct json_object_s* MetaObject = json_value_as_object(MetaObjectValue);

	if (MetaObject == NULL) {
		return false;
	}

	DataOut->ImageNameId = JsonGetStringId(MetaObject, "image", KStringIdInvalid);
	DataOut->FormatNameId = JsonGetStringId(MetaObject, "format", KStringIdInvalid);
	DataOut->Scale = JsonGetNumber(MetaObject, "scale", 1.0);
	bool ParsedSize = ParseDimensions(JsonFindKeyValue(MetaObject, "size"), &DataOut->Size);

	bool Success = StringIdIsValid(DataOut->ImageNameId) &&
				   StringIdIsValid(DataOut->FormatNameId) && ParsedSize;

	return Success;
}

static bool ParseSpriteSheetFrameData(
	struct json_value_s* FrameArrayValue,
	SpriteSheetFramesData* DataOut)
{
	ASSERT(DataOut);
	ZERO_STRUCT(DataOut);

	struct json_array_s* FrameArray = json_value_as_array(FrameArrayValue);

	if (FrameArray == NULL) {
		return false;
	}

	const int32 ExpectedSprites = FrameArray->length;

	struct json_array_element_s* Current = FrameArray->start;
	while (Current) {
		if (ParseSpriteFrameData(Current->value, DataOut, DataOut->Count)) {
			DataOut->Count++;
		}
		Current = Current->next;
	}

	return DataOut->Count == ExpectedSprites;
}

static bool ParseSpriteFrameData(
	struct json_value_s* FrameValue,
	SpriteSheetFramesData* DataOut,
	int32 Id)
{
	struct json_object_s* FrameObject = json_value_as_object(FrameValue);
	if (FrameObject == NULL) {
		return false;
	}

	bool Success = true;

	DataOut->Name[Id] = JsonGetStringId(FrameObject, "filename", KStringIdInvalid);

	Success &= ParseRect(JsonFindKeyValue(FrameObject, "frame"), &DataOut->Frame[Id]);
	Success &=
		ParseDimensions(JsonFindKeyValue(FrameObject, "sourceSize"), &DataOut->SourceSize[Id]);
	Success &= StringIdIsValid(DataOut->Name[Id]);

	DataOut->Rotated[Id] = JsonGetBool(FrameObject, "rotated", false);
	DataOut->Trimmed[Id] = JsonGetBool(FrameObject, "trimmed", false);

	return Success;
}

static bool ParseNumber(struct json_value_s* NumberValue, double* NumberOut)
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

static bool ParseBool(struct json_value_s* BoolValue, bool* BoolOut)
{
	ASSERT(BoolOut);
	*BoolOut = false;

	if (BoolValue == NULL) {
		return false;
	}

	*BoolOut = BoolValue->type == json_type_true;
	return (BoolValue->type == json_type_true || BoolValue->type == json_type_false);
}

static bool ParseStringId(struct json_value_s* StringValue, StringId* StringIdOut)
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

static bool ParseRect(struct json_value_s* RectValue, Rect* RectOut)
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

static bool ParseDimensions(struct json_value_s* DimValue, Point* DimOut)
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

static struct json_value_s* JsonFindKeyValue(struct json_object_s* Object, const char* Key)
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

static bool JsonGetBool(struct json_object_s* Object, const char* Key, bool Default)
{
	struct json_value_s* BoolValue = JsonFindKeyValue(Object, Key);

	bool Result;
	if (ParseBool(BoolValue, &Result)) {
		return Result;
	}
	return Default;
}

static StringId JsonGetStringId(struct json_object_s* Object, const char* Key, StringId Default)
{
	struct json_value_s* StringValue = JsonFindKeyValue(Object, Key);

	StringId Result;
	if (ParseStringId(StringValue, &Result)) {
		return Result;
	}
	return Default;
}

static double JsonGetNumber(struct json_object_s* Object, const char* Key, double Default)
{
	struct json_value_s* NumberValue = JsonFindKeyValue(Object, Key);

	double Result;
	if (ParseNumber(NumberValue, &Result)) {
		return Result;
	}
	return Default;
}

static int64 JsonGetInt64(struct json_object_s* Object, const char* Key, int64 Default)
{
	struct json_value_s* NumberValue = JsonFindKeyValue(Object, Key);

	double Result;
	if (ParseNumber(NumberValue, &Result)) {
		return (int64)Result;
	}
	return Default;
}

static int32 JsonGetInt32(struct json_object_s* Object, const char* Key, int32 Default)
{
	struct json_value_s* NumberValue = JsonFindKeyValue(Object, Key);

	double Result;
	if (ParseNumber(NumberValue, &Result)) {
		return (int32)Result;
	}
	return Default;
}
