#include "AssetTypes.h"

#include <SDL3/SDL.h>
#include <stb_ds.h>
#include <stb_image.h>

#include "JsonHelpers.h"

// Private Definitions

// Private Prototypes

static bool ParseSpriteSheetMetaData(struct json_value_s* MetaObjectValue, SpriteSheetMetaData* DataOut);
static bool ParseSpriteSheetFrameData(struct json_value_s* FrameArrayValue, SpriteSheetFramesData* DataOut);
static bool ParseSpriteFrameData(struct json_value_s* FrameValue, SpriteSheetFramesData* DataOut, int32 Id);

// Public Implementations

bool LoadImageData(const char* FileName, ImageData* DataOut)
{
	ASSERT(DataOut);

	ZERO_STRUCT(DataOut);

	// Hardcoding 4 bytes per pixel regardless of source because lazy
	int Width, Height, Channels;
	uint8* Pixels = stbi_load(FileName, &Width, &Height, &Channels, 4);

	if (Pixels == NULL) {
		return false;
	}

	SDL_Surface* Surface = SDL_CreateSurfaceFrom(Width, Height, SDL_PIXELFORMAT_ABGR8888, Pixels, Width * 4);

	if (Surface == NULL) {
		stbi_image_free(Pixels);
		return false;
	}

	DataOut->Pixels = Pixels;
	DataOut->Width = Surface->w;
	DataOut->Height = Surface->h;
	DataOut->BytesPerPixel = SDL_GetPixelFormatDetails(Surface->format)->bytes_per_pixel;
	DataOut->Pitch = Surface->pitch;
	DataOut->Surface = Surface;
	return true;
}

void UnloadImageData(ImageData* Data)
{
	SDL_DestroySurface(Data->Surface);
	stbi_image_free(Data->Pixels);
}

bool LoadSpriteSheetData(const char* FileName, SpriteSheetData* DataOut)
{
	ASSERT(DataOut);
	ZERO_STRUCT(DataOut);

	bool Success = true;

	struct json_value_s* ParsedJson = JsonLoadFile(FileName);
	struct json_object_s* Object = json_value_as_object(ParsedJson);

	if (Object == NULL || Object->length != 2) {
		Success = false;
		goto CleanUp;
	}

	bool MetaObjectParsed = ParseSpriteSheetMetaData(JsonFindKeyValue(Object, "meta"), &DataOut->Meta);
	bool FramesArrayParsed = ParseSpriteSheetFrameData(JsonFindKeyValue(Object, "frames"), &DataOut->Frames);

	if (!(MetaObjectParsed && FramesArrayParsed)) {
		Success = false;
		goto CleanUp;
	}

	hmdefault(DataOut->NameIdMap, NONE);
	for (int32 SpriteIndex = 0; SpriteIndex < DataOut->Frames.Count; SpriteIndex++) {
		StringId NameId = DataOut->Frames.Name[SpriteIndex];
		hmput(DataOut->NameIdMap, NameId, SpriteIndex);
	}

CleanUp:
	free(ParsedJson);
	return Success;
}

void UnloadSpriteSheetData(SpriteSheetData* Data)
{
	hmfree(Data->NameIdMap);
	ZERO_STRUCT(Data);
}

// Private Implementations

static bool ParseSpriteSheetMetaData(struct json_value_s* MetaObjectValue, SpriteSheetMetaData* DataOut)
{
	ASSERT(DataOut);
	ZERO_STRUCT(DataOut);

	struct json_object_s* MetaObject = json_value_as_object(MetaObjectValue);

	if (MetaObject == NULL) {
		return false;
	}

	DataOut->ImageNameId = JsonGetStringId(MetaObject, "image", KInvalidStringId);
	DataOut->FormatNameId = JsonGetStringId(MetaObject, "format", KInvalidStringId);
	DataOut->Scale = JsonGetNumber(MetaObject, "scale", 1.0);
	bool ParsedSize = JsonParseDimensions(JsonFindKeyValue(MetaObject, "size"), &DataOut->Size);

	bool Success = StringIdIsValid(DataOut->ImageNameId) && StringIdIsValid(DataOut->FormatNameId) && ParsedSize;

	return Success;
}

static bool ParseSpriteSheetFrameData(struct json_value_s* FrameArrayValue, SpriteSheetFramesData* DataOut)
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
			ASSERT(DataOut->Count < KSpriteSheetAssetMaxSprites);
			DataOut->Count++;
		}
		Current = Current->next;
	}

	return DataOut->Count == ExpectedSprites;
}

static bool ParseSpriteFrameData(struct json_value_s* FrameValue, SpriteSheetFramesData* DataOut, int32 Id)
{
	struct json_object_s* FrameObject = json_value_as_object(FrameValue);
	if (FrameObject == NULL) {
		return false;
	}

	bool Success = true;

	DataOut->Name[Id] = JsonGetStringId(FrameObject, "filename", KInvalidStringId);

	Success &= JsonParseRect16(JsonFindKeyValue(FrameObject, "frame"), &DataOut->Frame[Id]);
	Success &= JsonParseDimensions16(JsonFindKeyValue(FrameObject, "sourceSize"), &DataOut->SourceSize[Id]);
	Success &= StringIdIsValid(DataOut->Name[Id]);

	DataOut->Rotated[Id] = JsonGetBool(FrameObject, "rotated", false);
	DataOut->Trimmed[Id] = JsonGetBool(FrameObject, "trimmed", false);

	return Success;
}
