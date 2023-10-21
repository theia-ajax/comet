#pragma once

#include "Assets.h"
#include "Math.h"
#include "types.h"

typedef struct SDL_Renderer SDL_Renderer;

enum {
	KMaxDrawSpriteSheets = 16,
};

typedef struct SpriteSheet {
	ImageAsset* Image;
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
	int32 SpriteSheetId;
	int32 SpriteId;
	Vec2 Position;
	Vec2 Scale;
	real32 Rotation;
	int32 SpriteTiles[2];
} SpriteDraw;

SpriteSheet CreateSpriteSheet(ImageAsset* imageAsset, int32 spriteWidth, int32 spriteHeight);

void DrawInitialize(const DrawConfig* config);
void DrawShutdown(void);

void DrawSprite(const SpriteDraw* spriteDraw);
void DrawRender(void);