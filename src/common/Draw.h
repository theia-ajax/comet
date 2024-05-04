#pragma once

#include "AssetTypes.h"
#include "Math2D.h"
#include "SpriteDatabase.h"

typedef struct SDL_Renderer SDL_Renderer;

typedef struct DrawConfig {
	SDL_Renderer* Renderer;
} DrawConfig;

typedef struct ColorU8 { uint8 R, G, B, A; } ColorU8;

typedef struct SpriteDraw {
	SpriteId SpriteId;
	Vec2 Position;
	Vec2 Scale;
	flt32 Rotation;
	Point SpriteTiles;
	ColorU8 TintColor;
	bool UseTint;
	int32 Layer;
} SpriteDraw;

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
