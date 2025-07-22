#pragma once

#include "AssetTypes.h"

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

typedef enum SpriteSheetType {
	SpriteSheetType_None,
	SpriteSheetType_Grid,
	SpriteSheetType_Frames,
	SpriteSheetType_Count,
} SpriteSheetType;

typedef struct SpriteSheet {
	SpriteSheetType SheetType;
	ImageAsset* Image;
	SpriteSheetAsset* SheetData;
	SDL_Texture* Texture;
	StringId Name;
	int32 SpriteWidth;
	int32 SpriteHeight;
	int32 SpritesPerRow;
	int32 SpritesPerCol;
} SpriteSheet;
DEFINE_HANDLE(SpriteSheetId);

typedef struct SpriteRect {
	float32 X, Y, W, H;
} SpriteRect;
DEFINE_HANDLE(SpriteId);

void SpriteDatabaseInitialize(SDL_Renderer* Renderer);
void SpriteDatabaseShutdown(void);

SpriteSheetId SpriteDatabaseCreateGridSpriteSheet(StringId Name, ImageAsset* Image, int32 SpriteWidth, int32 SpriteHeight);
SpriteSheetId SpriteDatabaseCreateFrameDataSpriteSheet(StringId Name, ImageAsset* Image, SpriteSheetAsset* Sheet);

const SpriteSheet* SpriteDatabaseTryGetSpriteSheet(SpriteSheetId SpriteSheetHandle);
const SpriteSheet* SpriteDatabaseGetSpriteSheet(SpriteSheetId SpriteSheetHandle);

int32 SpriteSheetSpriteCount(SpriteSheetId SpriteSheetHandle);

SpriteId SpriteFindByName(const char* SpriteName);
SpriteId SpriteFindByNameId(StringId SpriteName);
SpriteId SpriteSheetFindSpriteByName(SpriteSheetId SpriteSheetHandle, const char* SpriteName);
SpriteId SpriteSheetFindSpriteByNameId(SpriteSheetId SpriteSheetHandle, StringId SpriteName);
SpriteId SpriteSheetFindSpriteByIndex(SpriteSheetId SpriteSheetHandle, int32 SpriteIndex);

SpriteSheetId SpriteSheetFindByName(StringId Name);

SDL_Texture* GetSpriteTexture(SpriteId SpriteHandle);
SpriteRect GetSpriteRect(SpriteId SpriteHandle);
