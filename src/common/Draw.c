#include "Draw.h"

#include <SDL2/SDL.h>
#include <json.h>

#include "Math2D.h"
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
	SpriteSheet SpriteSheets[KMaxDrawSpriteSheets];
	SDL_Texture* SpriteSheetTextures[KMaxDrawSpriteSheets];
	SpriteDraw SpriteQueue[KMaxSpriteDrawCalls];
	int32 SpriteCount;
	PrimDrawCmd PrimitiveQueue[KMaxPrimitiveDrawCalls];
	int32 PrimitiveCount;
} GDraw;

// Private Prototypes
static SDL_Rect GetSpriteRect(int32 SpriteId, int32 SpriteTilesX, int32 SpriteTilesY);

// Note SDL style naming convention
static void SDL_RenderDrawCircle(SDL_Renderer* renderer, const SDL_FPoint* center, float radius);

// Public Implementations

SpriteSheet CreateSpriteSheetGrid(ImageAsset* Image, int32 SpriteWidth, int32 SpriteHeight)
{
	SpriteSheet Result = {
		.SheetType = SpriteSheetType_Grid,
		.Image = Image,
		.SpriteWidth = SpriteWidth,
		.SpriteHeight = SpriteHeight,
	};

	Result.SpritesPerRow = Result.Image->Data->Surface->w / SpriteWidth;
	Result.SpritesPerCol = Result.Image->Data->Surface->h / SpriteHeight;

	return Result;
}

SpriteSheet CreateSpriteSheetFrameData(ImageAsset* Image, SpriteSheetAsset* Sheet)
{
	SpriteSheet Result = {
		.SheetType = SpriteSheetType_Frames,
		.Image = Image,
		.SheetData = Sheet,
	};

	return Result;
}

void DrawInitialize(const DrawConfig* config)
{
	ZERO_STRUCT(&GDraw);

	memcpy(GDraw.SpriteSheets, config->SpriteSheets, sizeof(GDraw.SpriteSheets));

	GDraw.Renderer = config->Renderer;

	for (int32 SpriteSheetIndex = 0; SpriteSheetIndex < KMaxDrawSpriteSheets; SpriteSheetIndex++) {
		const SpriteSheet* SpriteSheet = &config->SpriteSheets[SpriteSheetIndex];

		if (SpriteSheet->SheetType == SpriteSheetType_None || SpriteSheet->Image == NULL) {
			break;
		}

		GDraw.SpriteSheetTextures[SpriteSheetIndex] =
			SDL_CreateTextureFromSurface(GDraw.Renderer, SpriteSheet->Image->Data->Surface);
	}
}

void DrawShutdown(void)
{
	for (int32 TextureIndex = 0; TextureIndex < KMaxDrawSpriteSheets; TextureIndex++) {
		SDL_DestroyTexture(GDraw.SpriteSheetTextures[TextureIndex]);
	}
}

void DrawSprite(const SpriteDraw* spriteDraw)
{
	SpriteDraw DrawCommand = (spriteDraw != NULL) ? *spriteDraw : (SpriteDraw){0};

	if (EqV2(DrawCommand.Scale, V2(0, 0))) {
		DrawCommand.Scale = V2(1, 1);
	}

	DrawCommand.SpriteTiles[0] = (DrawCommand.SpriteTiles[0] > 0) ? DrawCommand.SpriteTiles[0] : 1;
	DrawCommand.SpriteTiles[1] = (DrawCommand.SpriteTiles[1] > 0) ? DrawCommand.SpriteTiles[1] : 1;

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

void DrawRender(void)
{
	for (int32 SpriteDrawIndex = 0; SpriteDrawIndex < GDraw.SpriteCount; SpriteDrawIndex++) {
		const SpriteDraw* DrawCommand = &GDraw.SpriteQueue[SpriteDrawIndex];

		SDL_Rect SourceRect =
			GetSpriteRect(DrawCommand->SpriteId, DrawCommand->SpriteTiles[0], DrawCommand->SpriteTiles[1]);

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

		int32 SheetIndex = SPRITE_ID_SHEET(DrawCommand->SpriteId);

		SDL_Texture* Texture = GDraw.SpriteSheetTextures[SheetIndex];

		if (DrawCommand->UseTint) {
			uint32 TintColor = DrawCommand->TintColor;
			uint R = (TintColor >> 0) & 0xFF;
			uint G = (TintColor >> 8) & 0xFF;
			uint B = (TintColor >> 16) & 0xFF;
			SDL_SetTextureColorMod(Texture, R, G, B);
		}

		// for now it's all in the first sprite sheet
		SDL_RenderCopyExF(
			GDraw.Renderer,
			GDraw.SpriteSheetTextures[SheetIndex],
			&SourceRect,
			&DestRect,
			DrawCommand->Rotation * TurnToDeg,
			&Center,
			SDL_FLIP_NONE);

		if (DrawCommand->UseTint) {
			SDL_SetTextureColorMod(Texture, 255, 255, 255);
		}

		// SDL_FRect PosRect = (SDL_FRect){
		// 	.x = DestRect.x + Center.x,
		// 	.y = DestRect.y + Center.y,
		// 	.w = 1.0f,
		// 	.h = 1.0f,
		// };
		// SDL_SetRenderDrawColor(GDraw.Renderer, 0, 255, 255, 0);
		// SDL_RenderDrawRectF(GDraw.Renderer, &PosRect);
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
					GDraw.Renderer, (SDL_FPoint*)&DrawCmd->PrimCircle.Center, DrawCmd->PrimCircle.Radius);
				break;
			case KShapePolygon:
				{
					SDL_FPoint Points[KPolygonMaxVerts + 1];
					memcpy(
						Points, DrawCmd->PrimPolygon.Vertices, DrawCmd->PrimPolygon.VertexCount * sizeof(SDL_FPoint));
					Points[DrawCmd->PrimPolygon.VertexCount] = Points[0];
					const int32 Count = DrawCmd->PrimPolygon.VertexCount;
					for (int32 EdgeIndex = 0; EdgeIndex < Count; EdgeIndex++) {
						flt32 EdgeRatio = (flt32)EdgeIndex / Count;
						Vec4 ColorF = Lerp(ColorF0, ColorF1, EdgeRatio);
						ColorV4ToBytes(ColorF, &R, &G, &B, &A);
						SDL_SetRenderDrawColor(GDraw.Renderer, R, G, B, A);
						SDL_RenderDrawLineF(
							GDraw.Renderer,
							Points[EdgeIndex].x,
							Points[EdgeIndex].y,
							Points[EdgeIndex + 1].x,
							Points[EdgeIndex + 1].y);
					}
					// SDL_RenderDrawLinesF(GDraw.Renderer, Points, DrawCmd->PrimPolygon.VertexCount + 1);
				}
				break;
			default:
				break;
		}
	}

	GDraw.SpriteCount = 0;
	GDraw.PrimitiveCount = 0;
}

// Private Implementations

static SDL_Rect GetSpriteRect(int32 SpriteId, int32 SpriteTilesX, int32 SpriteTilesY)
{
	const SpriteSheet* SpriteSheet = &GDraw.SpriteSheets[SPRITE_ID_SHEET(SpriteId)];

	SDL_Rect Result;

	switch (SpriteSheet->SheetType) {
		case SpriteSheetType_Grid:
			{
				int32 SpriteIndex = SPRITE_ID_INDEX(SpriteId);
				int32 SpriteTileX = SpriteIndex % SpriteSheet->SpritesPerRow;
				int32 SpriteTileY = SpriteIndex / SpriteSheet->SpritesPerRow;

				if (SpriteTileX + SpriteTilesX > SpriteSheet->SpritesPerRow)
					SpriteTilesX = SpriteSheet->SpritesPerRow - SpriteTileX;

				if (SpriteTileY + SpriteTilesY > SpriteSheet->SpritesPerCol)
					SpriteTilesY = SpriteSheet->SpritesPerCol - SpriteTileY;

				Result = (SDL_Rect){
					.x = SpriteTileX * SpriteSheet->SpriteWidth,
					.y = SpriteTileY * SpriteSheet->SpriteHeight,
					.w = SpriteTilesX * SpriteSheet->SpriteWidth,
					.h = SpriteTilesY * SpriteSheet->SpriteHeight,
				};
			}
			break;
		case SpriteSheetType_Frames:
			{
				Rect16 R = SpriteSheet->SheetData->Data->Frames.Frame[SPRITE_ID_INDEX(SpriteId)];
				Result = (SDL_Rect){R.X, R.Y, R.W, R.H};
			}
			break;
		default:
			break;
	}

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

	SDL_RenderDrawLinesF(renderer, points, SDL_RENDER_CIRCLE_SEGMENTS + 1);
}
