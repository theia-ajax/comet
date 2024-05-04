#include "Draw.h"

#include <SDL3/SDL.h>
#include <json.h>

#include "Log.h"
#include "Math2D.h"
#include "SpriteDatabase.h"
#include "Util.h"

// Constants

enum {
	KMaxSpriteDrawCalls = 4096 * 16,
	KMaxPrimitiveDrawCalls = 4096,
};

enum { KShapeCircle = 0, KShapePolygon = 1 };

// Private Definitions
typedef struct PrimDrawCmd {
	uint32 Color;
	int32 Shape;
	CircleShape PrimCircle;
	PolygonShape PrimPolygon;
} PrimDrawCmd;

struct {
	SDL_Renderer* Renderer;
	SpriteDraw SpriteQueue[KMaxSpriteDrawCalls];
	int32 SpriteCount;
	PrimDrawCmd PrimitiveQueue[KMaxPrimitiveDrawCalls];
	int32 PrimitiveCount;
} GDraw;

// Private Prototypes
static SDL_FRect GetSpriteSDLRect(SpriteId Sprite, Point SpriteTiles);

// Note SDL style naming convention
static void SDL_RenderDrawCircle(SDL_Renderer* renderer, const SDL_FPoint* center, float radius);

// Public Implementations

void DrawInitialize(const DrawConfig* config)
{
	ZERO_STRUCT(&GDraw);

	GDraw.Renderer = config->Renderer;

	LogInfo(__FUNCTION__);
}

void DrawShutdown(void)
{
	LogInfo(__FUNCTION__);
}

void DrawSprite(const SpriteDraw* spriteDraw)
{
	SpriteDraw DrawCommand = (spriteDraw != NULL) ? *spriteDraw : (SpriteDraw){0};

	if (EqV2(DrawCommand.Scale, V2(0, 0))) {
		DrawCommand.Scale = V2(1, 1);
	}

	DrawCommand.SpriteTiles.X = (DrawCommand.SpriteTiles.X > 0) ? DrawCommand.SpriteTiles.X : 1;
	DrawCommand.SpriteTiles.Y = (DrawCommand.SpriteTiles.Y > 0) ? DrawCommand.SpriteTiles.Y : 1;

	if (GDraw.SpriteCount < KMaxSpriteDrawCalls) {
		GDraw.SpriteQueue[GDraw.SpriteCount++] = DrawCommand;
	}
}

void DrawCircle(Vec2 Center, flt32 Radius, uint32 Color)
{
	if (GDraw.PrimitiveCount < KMaxPrimitiveDrawCalls) {
		GDraw.PrimitiveQueue[GDraw.PrimitiveCount++] = (PrimDrawCmd){
			.Shape = KShapeCircle,
			.Color = Color,
			.PrimCircle =
				(CircleShape){
					.Center = Center,
					.Radius = Radius,
				},
		};
	}
}

void DrawAABB(AABB AABB_, uint32 Color)
{
	Vec2 Verts[4] = {
		AABB_.MinBound,
		V2(AABB_.MaxBound.X, AABB_.MinBound.Y),
		AABB_.MaxBound,
		V2(AABB_.MinBound.X, AABB_.MaxBound.Y),
	};
	DrawPolygon(V2(0, 0), R2Ident(), Verts, 4, Color);
}

void DrawPolygon(Vec2 TxPos, Rot2 TxRot, const Vec2* Verts, int32 Count, uint32 Color)
{
	if (GDraw.PrimitiveCount < KMaxPrimitiveDrawCalls) {
		PolygonShape P;
		ASSERT(Count <= ARRAY_COUNT(P.Vertices));
		memcpy(P.Vertices, Verts, Count * sizeof(Vec2));
		P.VertexCount = Count;
		for (int32 Index = 0; Index < P.VertexCount; Index++) {
			P.Vertices[Index] = TransformV2(T2(TxPos, TxRot), P.Vertices[Index]);
		}
		GDraw.PrimitiveQueue[GDraw.PrimitiveCount++] = (PrimDrawCmd){
			.Shape = KShapePolygon,
			.Color = Color,
			.PrimPolygon = P,
		};
	}
}

static int SpriteDrawLayerCompare(const SpriteDraw* A, const SpriteDraw* B)
{
	return A->Layer - B->Layer;
}

static int SpriteDrawLayerCompareVoid(const void* A, const void* B)
{
	return SpriteDrawLayerCompare((const SpriteDraw*)A, (const SpriteDraw*)B);
}

void DrawRender(void)
{
	SDL_qsort(GDraw.SpriteQueue, GDraw.SpriteCount, sizeof(GDraw.SpriteQueue[0]), SpriteDrawLayerCompareVoid);

	for (int32 SpriteDrawIndex = 0; SpriteDrawIndex < GDraw.SpriteCount; SpriteDrawIndex++) {
		const SpriteDraw* DrawCommand = &GDraw.SpriteQueue[SpriteDrawIndex];

		SDL_FRect SourceRect = GetSpriteSDLRect(DrawCommand->SpriteId, DrawCommand->SpriteTiles);

		float Width = SourceRect.w * DrawCommand->Scale.X;
		float Height = SourceRect.h * DrawCommand->Scale.Y;

		SDL_FRect DestRect = {
			.x = DrawCommand->Position.X - Width / 2.0f,
			.y = DrawCommand->Position.Y - Height / 2.0f,
			.w = Width,
			.h = Height,
		};

		SDL_FPoint Center = {
			DestRect.w / 2.0f,
			DestRect.h / 2.0f,
		};

		SDL_Texture* Texture = GetSpriteTexture(DrawCommand->SpriteId);

		if (DrawCommand->UseTint) {
			ColorU8 TintColor = DrawCommand->TintColor;
			SDL_SetTextureColorMod(Texture, TintColor.R, TintColor.G, TintColor.B);
		}

		SDL_RenderTextureRotated(
			GDraw.Renderer,
			Texture,
			&SourceRect,
			&DestRect,
			DrawCommand->Rotation * TurnToDeg,
			&Center,
			SDL_FLIP_NONE);

		if (DrawCommand->UseTint) {
			SDL_SetTextureColorMod(Texture, 255, 255, 255);
		}
	}

	for (int32 PrimIndex = 0; PrimIndex < GDraw.PrimitiveCount; PrimIndex++) {
		const PrimDrawCmd* DrawCmd = &GDraw.PrimitiveQueue[PrimIndex];
		uint32 Color = DrawCmd->Color;
		uint8 R = (Color >> 0) & 0xFF;
		uint8 G = (Color >> 8) & 0xFF;
		uint8 B = (Color >> 16) & 0xFF;
		uint8 A = (Color >> 24);
		SDL_SetRenderDrawColor(GDraw.Renderer, R, G, B, A);

		Vec4 ColorF0 = V4(R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f);
		flt32 Grey = VecSort(ColorF0.RGB).G;
		Vec4 ColorF1 = V4V(Splat(Grey).RGB, A);

		switch (DrawCmd->Shape) {
			case KShapeCircle:
				SDL_RenderDrawCircle(
					GDraw.Renderer,
					(SDL_FPoint*)&DrawCmd->PrimCircle.Center,
					DrawCmd->PrimCircle.Radius);
				break;
			case KShapePolygon:
				{
					SDL_FPoint Points[KPolygonMaxVerts + 1];
					memcpy(Points, DrawCmd->PrimPolygon.Vertices, DrawCmd->PrimPolygon.VertexCount * sizeof(SDL_FPoint));
					Points[DrawCmd->PrimPolygon.VertexCount] = Points[0];
					const int32 Count = DrawCmd->PrimPolygon.VertexCount;
					for (int32 EdgeIndex = 0; EdgeIndex < Count; EdgeIndex++) {
						flt32 EdgeRatio = (flt32)EdgeIndex / Count;
						Vec4 ColorF = Lerp(ColorF0, ColorF1, EdgeRatio);
						ColorV4ToBytes(ColorF, &R, &G, &B, &A);
						SDL_SetRenderDrawColor(GDraw.Renderer, R, G, B, A);
						SDL_RenderLine(
							GDraw.Renderer,
							Points[EdgeIndex].x,
							Points[EdgeIndex].y,
							Points[EdgeIndex + 1].x,
							Points[EdgeIndex + 1].y);
					}
					// SDL_RenderDrawLinesF(GDraw.Renderer, Points, DrawCmd->PrimPolygon.VertexCount + 1);
				}
				break;
			default: break;
		}
	}

	GDraw.SpriteCount = 0;
	GDraw.PrimitiveCount = 0;
}

// Private Implementations

static SDL_FRect GetSpriteSDLRect(SpriteId Sprite, Point SpriteTiles)
{
	SpriteRect Rect = GetSpriteRect(Sprite);
	SDL_FRect Result = *((SDL_FRect*)&Rect);
	Result.w *= SpriteTiles.X;
	Result.h *= SpriteTiles.Y;
	return Result;
}

static void SDL_RenderDrawCircle(SDL_Renderer* renderer, const SDL_FPoint* center, float radius)
{
	enum { SDL_RENDER_CIRCLE_SEGMENTS = 11 };
	SDL_FPoint points[SDL_RENDER_CIRCLE_SEGMENTS + 1];

	const float td = M_PI * 2 / SDL_RENDER_CIRCLE_SEGMENTS;
	for (int32 i = 0; i < SDL_RENDER_CIRCLE_SEGMENTS; i++) {
		SDL_FPoint* p = &points[i];
		float t = i * td;
		p->x = cosf(t) * radius + center->x;
		p->y = sinf(t) * radius + center->y;
	}
	points[SDL_RENDER_CIRCLE_SEGMENTS] = points[0];

	SDL_RenderLines(renderer, points, SDL_RENDER_CIRCLE_SEGMENTS + 1);
}

void ColorV4ToBytes(Vec4 Color, uint8* R, uint8* G, uint8* B, uint8* A)
{
	if (R != NULL) *R = (uint8)round(Color.R * 255.0f);
	if (G != NULL) *G = (uint8)round(Color.G * 255.0f);
	if (B != NULL) *B = (uint8)round(Color.B * 255.0f);
	if (A != NULL) *A = (uint8)round(Color.A * 255.0f);
}

ColorU8 ColorV4ToColorU8(Vec4 Color)
{
	ColorU8 Result;
	Result.R = (uint8)round(Color.R * 255.0f);
	Result.G = (uint8)round(Color.G * 255.0f);
	Result.B = (uint8)round(Color.B * 255.0f);
	Result.A = (uint8)round(Color.A * 255.0f);
	return Result;
}

ColorU8 HsvToColorU8(flt32 H, flt32 S, flt32 V, flt32 A)
{
	if (S == 0.0f) {
		uint8 V8 = (uint8)(V * 255.0f);
		uint8 A8 = (uint8)(A * 255.0f);
		return (ColorU8){V8, V8, V8, A8};
	}

	H = (H - floor(H)) * 6.0f;
	int32 HI = (int32)H;
	flt32 Frac = H - HI;
	flt32 N0 = V * (1.0f - S);
	flt32 N1 = V * (1.0f - S * Frac);
	flt32 N2 = V * (1.0f - S * (1.0f - Frac));

	flt32 RP = V, GP = V, BP = V;
	// clang-format off
	switch (HI)
	{
		case 0: GP = N2; BP = N0; break;
		case 1: RP = N1; BP = N0; break;
		case 2: RP = N0; BP = N2; break;
		case 3: RP = N0; GP = N1; break;
		case 4: RP = N2; GP = N0; break;
		case 5: GP = N0; BP = N1; break;
		default: unreachable(); break;
	}
	// clang-format on

	return ColorV4ToColorU8(V4(RP, GP, BP, A));
}
