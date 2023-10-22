#pragma once

#include "Math.h"
#include "StringId.h"
#include "types.h"

typedef struct SDL_Surface SDL_Surface;

typedef struct AssetsConfig {

} AssetsConfig;

typedef struct ImageAsset {
	uint8* Pixels;
	SDL_Surface* Surface;
	int32 Width;
	int32 Height;
	int32 Pitch;
} ImageAsset;

enum {
	KSpriteSheetAssetMaxSprites = 256,
};

typedef struct SpriteSheetFramesData {
	int32 Count;
	StringId Name[KSpriteSheetAssetMaxSprites];
	Rect Frame[KSpriteSheetAssetMaxSprites];
	Point SourceSize[KSpriteSheetAssetMaxSprites];
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

typedef struct SpriteSheetDataAsset {
	SpriteSheetFramesData Frames;
	SpriteSheetMetaData Meta;
	SpriteNameIdMap* NameIdMap;
} SpriteSheetDataAsset;

void AssetsInitialize(const AssetsConfig* config);
void AssetsShutdown(void);

ImageAsset* LoadImageAsset(const char* fileName);
void UnloadImageAsset(ImageAsset* image);

SpriteSheetDataAsset* LoadSpriteSheetDataAsset(const char* fileName);
void UnloadSpriteSheetDataAsset(SpriteSheetDataAsset* spriteSheet);
