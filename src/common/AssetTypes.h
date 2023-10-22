#pragma once

#include "Assets.h"

typedef struct ImageData {
	uint8* Pixels;
	SDL_Surface* Surface;
} ImageData;

enum {
	KSpriteSheetAssetMaxSprites = 256,
};

typedef struct SpriteSheetFramesData {
	int32 Count;
	StringId Name[KSpriteSheetAssetMaxSprites];
	Rect16 Frame[KSpriteSheetAssetMaxSprites];
	Point16 SourceSize[KSpriteSheetAssetMaxSprites];
	bool Rotated[KSpriteSheetAssetMaxSprites];
	bool Trimmed[KSpriteSheetAssetMaxSprites];
} SpriteSheetFramesData;

typedef struct SpriteSheetMetaData {
	StringId ImageNameId;
	StringId FormatNameId;
	Point Size;
	real64 Scale;
} SpriteSheetMetaData;

typedef struct SpriteNameIdMap SpriteNameIdMap;

typedef struct SpriteSheetData {
	SpriteSheetFramesData Frames;
	SpriteSheetMetaData Meta;
	SpriteNameIdMap* NameIdMap;
} SpriteSheetData;

DEFINE_ASSET(Image);
DEFINE_ASSET(SpriteSheet);

bool LoadImageData(const char* FileName, ImageData* DataOut);
void UnloadImageData(ImageData* Data);

bool LoadSpriteSheetData(const char* FileName, SpriteSheetData* DataOut);
void UnloadSpriteSheetData(SpriteSheetData* Data);