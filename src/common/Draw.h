#pragma once

#include "AssetTypes.h"
#include "Math2D.h"

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

typedef struct ColorU8 { uint8 R, G, B, A; } ColorU8;

typedef struct SpriteDraw {
	int32 SpriteId;
	Vec2 Position;
	Vec2 Scale;
	flt32 Rotation;
	Point SpriteTiles;
	ColorU8 TintColor;
	bool UseTint;
	int32 Layer;
} SpriteDraw;

#define SPRITE_ID(Sheet, Sprite) (((Sheet) << 16) | ((Sprite) & 0xFFFF))
#define SPRITE_ID_SHEET(SpriteId) ((SpriteId) >> 16)
#define SPRITE_ID_INDEX(SpriteId) ((SpriteId) & 0xFFFF)

SpriteSheet CreateSpriteSheetGrid(ImageAsset* Image, int32 SpriteWidth, int32 SpriteHeight);
SpriteSheet CreateSpriteSheetFrameData(ImageAsset* Image, SpriteSheetAsset* Sheet);

void DrawInitialize(const DrawConfig* config);
void DrawShutdown(void);

void DrawSprite(const SpriteDraw* spriteDraw);
void DrawCircle(Vec2 Center, flt32 Radius, uint32 Color);
void DrawAABB(AABB AABB_, uint32 Color);
void DrawPolygon(Vec2 TxPos, Rot2 TxRot, const Vec2* Verts, int32 Count, uint32 Color);
void DrawRender(void);

void ColorV4ToBytes(Vec4 Color, uint8* R, uint8* G, uint8* B, uint8* A);
ColorU8 ColorV4ToColorU8(Vec4 Color);
ColorU8 HsvToColorU8(flt32 H, flt32 S, flt32 V, flt32 A);
