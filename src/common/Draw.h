#pragma once

#include "AssetTypes.h"
#include "Math2D.h"
#include "SpriteDatabase.h"
#include "ColorUtil.h"

typedef struct SDL_Renderer SDL_Renderer;

typedef struct DrawConfig {
	SDL_Renderer* Renderer;
} DrawConfig;

typedef struct SpriteDraw {
	SpriteId SpriteId;
	Vec2 Position;
	Vec2 Scale;
	float32 Rotation;
	Point SpriteTiles;
	ColorU8 TintColor;
	bool UseTint;
	float32 Layer;
} SpriteDraw;

void DrawInitialize(const DrawConfig* config);
void DrawShutdown(void);

void DrawSprite(const SpriteDraw* spriteDraw);
void DrawCircle(Vec2 Center, float32 Radius, uint32 Color);
void DrawAABB(AABB AABB_, uint32 Color);
void DrawPolygon(Vec2 TxPos, Rot2 TxRot, const Vec2* Verts, int32 Count, uint32 Color);
void DrawRender(void);


