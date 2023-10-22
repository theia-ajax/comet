#include "Assets.h"

#include <SDL2/SDL.h>
#include <stb_ds.h>
#include <stb_image.h>

#include "JsonHelpers.h"
#include "util.h"

// Private Definitions

typedef int32 FreeStack;

struct {
	AssetsConfig Config;
	ImageAsset* ImageAssets;
	FreeStack* ImageAssetsFreeStack;
	SpriteSheetDataAsset* SpriteSheetAssets;
} GAssets;

typedef struct SpriteNameIdMap {
	StringId Key;
	int32 Value;
} SpriteNameIdMap;

// Private Prototypes

static ImageAsset* AllocateImageAsset(void);
static void ReleaseImageAsset(ImageAsset* image);
static void FreeImageAssetResources(ImageAsset* image);

static SpriteSheetDataAsset* AllocateSpriteSheetAsset(void);
static void _ReleaseLastAllocatedSpriteSheetAsset(void);
static void FreeSpriteSheetAssetResources(SpriteSheetDataAsset* SpriteSheet);

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

SpriteSheetDataAsset* LoadSpriteSheetDataAsset(const char* fileName)
{
	SpriteSheetDataAsset* Result = NULL;
	struct json_value_s* ParsedJson = JsonLoadFile(fileName);
	struct json_object_s* Object = json_value_as_object(ParsedJson);

	if (Object == NULL || Object->length != 2) {
		goto CleanUp;
	}

	Result = AllocateSpriteSheetAsset();

	if (Result == NULL) {
		goto CleanUp;
	}

	bool MetaObjectParsed =
		ParseSpriteSheetMetaData(JsonFindKeyValue(Object, "meta"), &Result->Meta);
	bool FramesArrayParsed =
		ParseSpriteSheetFrameData(JsonFindKeyValue(Object, "frames"), &Result->Frames);

	if (!(MetaObjectParsed && FramesArrayParsed)) {
		Result = NULL;
		_ReleaseLastAllocatedSpriteSheetAsset();
		goto CleanUp;
	}

	for (int32 SpriteId = 0; SpriteId < Result->Frames.Count; SpriteId++) {
		StringId NameId = Result->Frames.Name[SpriteId];
		hmput(Result->NameIdMap, NameId, SpriteId);
	}

	size_t len = hmlen(Result->NameIdMap);

CleanUp:
	free(ParsedJson);
	return Result;
}

// Private Implementations

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

static SpriteSheetDataAsset* AllocateSpriteSheetAsset(void)
{
	arrput(GAssets.SpriteSheetAssets, (SpriteSheetDataAsset){0});
	return arrlastp(GAssets.SpriteSheetAssets);
}

static void _ReleaseLastAllocatedSpriteSheetAsset(void)
{
	arrpop(GAssets.SpriteSheetAssets);
}

static void FreeSpriteSheetAssetResources(SpriteSheetDataAsset* SpriteSheet)
{
	hmfree(SpriteSheet->NameIdMap);
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
	bool ParsedSize = JsonParseDimensions(JsonFindKeyValue(MetaObject, "size"), &DataOut->Size);

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

	Success &= JsonParseRect(JsonFindKeyValue(FrameObject, "frame"), &DataOut->Frame[Id]);
	Success &=
		JsonParseDimensions(JsonFindKeyValue(FrameObject, "sourceSize"), &DataOut->SourceSize[Id]);
	Success &= StringIdIsValid(DataOut->Name[Id]);

	DataOut->Rotated[Id] = JsonGetBool(FrameObject, "rotated", false);
	DataOut->Trimmed[Id] = JsonGetBool(FrameObject, "trimmed", false);

	return Success;
}
