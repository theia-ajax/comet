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
	int32 SpriteIndex);
static double ParseNumber(struct json_value_s* NumberValue);
static bool ParseBool(struct json_value_s* BoolValue, bool* BoolOut);
static bool ParseStringId(struct json_value_s* StringValue, StringId* StringIdOut);
static bool ParseRect(struct json_value_s* RectValue, Rect* RectOut);
static bool ParseDimensions(struct json_value_s* DimValue, Point* DimOut);

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

	enum { KExpectedKeys = 4 };
	int32 ParsedKeys = 0;

	struct json_object_element_s* Current = MetaObject->start;

	while (Current != NULL && ParsedKeys != KExpectedKeys) {
		struct json_value_s* Value = Current->value;
		if (JsonKeyEq(Current, "image") && ParseStringId(Value, &DataOut->ImageNameId)) {
			ParsedKeys++;
		} else if (JsonKeyEq(Current, "format") && ParseStringId(Value, &DataOut->FormatNameId)) {
			ParsedKeys++;
		} else if (JsonKeyEq(Current, "size") && ParseDimensions(Value, &DataOut->Size)) {
			ParsedKeys++;
		} else if (JsonKeyEq(Current, "scale")) {
			DataOut->Scale = ParseNumber(Value);
			ParsedKeys++;
		}

		Current = Current->next;
	}

	return ParsedKeys == KExpectedKeys;
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
	int32 SpriteIndex)
{
	bool Success = true;

	struct json_object_s* FrameObject = json_value_as_object(FrameValue);
	if (FrameObject == NULL) {
		return false;
	}

	enum { KExpectedKeys = 5 };
	int32 ParsedKeys = 0;

	int32 Id = SpriteIndex;

	struct json_object_element_s* Current = FrameObject->start;
	while (Current) {
		struct json_value_s* Value = Current->value;
		if (JsonKeyEq(Current, "filename") && ParseStringId(Value, &DataOut->Name[Id]))
			ParsedKeys++;
		else if (JsonKeyEq(Current, "frame") && ParseRect(Value, &DataOut->Frame[Id]))
			ParsedKeys++;
		else if (JsonKeyEq(Current, "sourceSize") && ParseDimensions(Value, &DataOut->SourceSize[Id]))
			ParsedKeys++;
		else if (JsonKeyEq(Current, "rotated") && ParseBool(Value, &DataOut->Rotated[Id]))
			ParsedKeys++;
		else if (JsonKeyEq(Current, "trimmed") && ParseBool(Value, &DataOut->Trimmed[Id]))
			ParsedKeys++;

		Current = Current->next;
	}

	return ParsedKeys == KExpectedKeys;
}

static double ParseNumber(struct json_value_s* NumberValue)
{
	double Result = 0.0;
	struct json_number_s* NumberObject = json_value_as_number(NumberValue);
	if (NumberObject != NULL) {
		Result = strtod(NumberObject->number, NULL);
	} else {
		struct json_string_s* StringObject = json_value_as_string(NumberValue);
		if (StringObject != NULL) {
			Result = strtod(StringObject->string, NULL);
		}
	}
	return Result;
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

	if (RectObject == NULL) {
		return false;
	}

	Rect Result = {0};

	enum { KExpectedKeys = 4 };
	int32 ParsedKeys = 0;

	struct json_object_element_s* Current = RectObject->start;
	while (Current && ParsedKeys != KExpectedKeys) {
		if (JsonKeyEq(Current, "x")) {
			Result.X = (int32)ParseNumber(Current->value);
			ParsedKeys++;
		}
		if (JsonKeyEq(Current, "y")) {
			Result.Y = (int32)ParseNumber(Current->value);
			ParsedKeys++;
		}
		if (JsonKeyEq(Current, "w")) {
			Result.W = (int32)ParseNumber(Current->value);
			ParsedKeys++;
		}
		if (JsonKeyEq(Current, "h")) {
			Result.H = (int32)ParseNumber(Current->value);
			ParsedKeys++;
		}
		Current = Current->next;
	}

	*RectOut = Result;

	return ParsedKeys == KExpectedKeys;
}

static bool ParseDimensions(struct json_value_s* DimValue, Point* DimOut)
{
	ASSERT(DimOut);
	ZERO_STRUCT(DimOut);

	struct json_object_s* DimObject = json_value_as_object(DimValue);

	if (DimObject == NULL) {
		return false;
	}

	Point Result = {0};

	enum { KExpectedKeys = 2 };
	int32 ParsedKeys = 0;

	struct json_object_element_s* Current = DimObject->start;
	while (Current && ParsedKeys != KExpectedKeys) {
		if (JsonKeyEq(Current, "w")) {
			Result.X = (int32)ParseNumber(Current->value);
			ParsedKeys++;
		}
		if (JsonKeyEq(Current, "h")) {
			Result.Y = (int32)ParseNumber(Current->value);
			ParsedKeys++;
		}
		Current = Current->next;
	}

	*DimOut = Result;

	return ParsedKeys == KExpectedKeys;
}