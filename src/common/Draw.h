#pragma once

#include "AssetTypes.h"
#include "Math.h"

typedef struct SDL_Renderer SDL_Renderer;

enum {
	KMaxDrawSpriteSheets = 16,
	KMaxSpritesPerSheet = 512,
};

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
	int32 SpriteWidth;
	int32 SpriteHeight;
	int32 SpritesPerRow;
	int32 SpritesPerCol;
} SpriteSheet;

typedef struct DrawConfig {
	SDL_Renderer* Renderer;
	SpriteSheet SpriteSheets[KMaxDrawSpriteSheets];
} DrawConfig;

typedef struct SpriteDraw {
	int32 SpriteId;
	Vec2 Position;
	Vec2 Scale;
	real32 Rotation;
	int32 SpriteTiles[2];
} SpriteDraw;

#define SPRITE_ID(Sheet, Sprite) (((Sheet) << 16) | ((Sprite) & 0xFFFF))
#define SPRITE_ID_SHEET(SpriteId) ((SpriteId) >> 16)
#define SPRITE_ID_INDEX(SpriteId) ((SpriteId) & 0xFFFF)

SpriteSheet CreateSpriteSheetGrid(ImageAsset* Image, int32 SpriteWidth, int32 SpriteHeight);
SpriteSheet CreateSpriteSheetFrameData(ImageAsset* Image, SpriteSheetAsset* Sheet);

void DrawInitialize(const DrawConfig* config);
void DrawShutdown(void);

void DrawSprite(const SpriteDraw* spriteDraw);
void DrawRender(void);
