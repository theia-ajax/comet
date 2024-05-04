#pragma once

#include "Assets.h"

// -------- ImageAsset

typedef struct ImageData {
	uint8* Pixels;
	SDL_Surface* Surface;
} ImageData;
DEFINE_ASSET(Image);

enum {
	KSpriteSheetAssetMaxSprites = 256,
};

bool LoadImageData(const char* FileName, ImageData* DataOut);
void UnloadImageData(ImageData* Data);

// -------- SpriteSheetAsset

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
	flt64 Scale;
} SpriteSheetMetaData;

typedef struct SpriteNameIdMap {
	StringId Key;
	int32 Value;
} SpriteNameIdMap;

typedef struct SpriteSheetData {
	SpriteSheetFramesData Frames;
	SpriteSheetMetaData Meta;
	SpriteNameIdMap* NameIdMap;
} SpriteSheetData;
DEFINE_ASSET(SpriteSheet);

bool LoadSpriteSheetData(const char* FileName, SpriteSheetData* DataOut);
void UnloadSpriteSheetData(SpriteSheetData* Data);
