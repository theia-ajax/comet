#include "Draw.h"

#include <SDL2/SDL.h>

// Constants

enum {
	KMaxSpriteDrawCalls = 4096,
};

// Private Definitions

struct {
	SDL_Renderer* Renderer;
	SpriteSheet SpriteSheets[KMaxDrawSpriteSheets];
	SDL_Texture* SpriteSheetTextures[KMaxDrawSpriteSheets];
	SpriteDraw SpriteQueue[KMaxSpriteDrawCalls];
	int32 SpriteCount;
} GDraw;

// Private Prototypes
static SDL_Rect GetSpriteRect(
	const SpriteSheet* spriteSheet,
	int32 spriteId,
	int32 spriteTilesX,
	int32 spriteTilesY);

// Public Implementations

SpriteSheet CreateSpriteSheet(ImageAsset* imageAsset, int32 SpriteWidth, int32 SpriteHeight)
{
	SpriteSheet Result = {
		.Image = imageAsset,
		.SpriteWidth = SpriteWidth,
		.SpriteHeight = SpriteHeight,
	};

	Result.SpritesPerRow = Result.Image->Width / SpriteWidth;
	Result.SpritesPerCol = Result.Image->Height / SpriteHeight;

	return Result;
}

void DrawInitialize(const DrawConfig* config)
{
	ZERO_STRUCT(&GDraw);

	memcpy(GDraw.SpriteSheets, config->SpriteSheets, sizeof(GDraw.SpriteSheets));

	GDraw.Renderer = config->Renderer;

	for (int32 SpriteSheetIndex = 0; SpriteSheetIndex < KMaxDrawSpriteSheets; SpriteSheetIndex++) {
		const SpriteSheet* SpriteSheet = &config->SpriteSheets[SpriteSheetIndex];

		if (SpriteSheet->Image == NULL || SpriteSheet->Image->Surface == NULL) {
			break;
		}

		GDraw.SpriteSheetTextures[SpriteSheetIndex] =
			SDL_CreateTextureFromSurface(GDraw.Renderer, SpriteSheet->Image->Surface);
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

	DrawCommand.SpriteTiles[0] = (DrawCommand.SpriteTiles[0] > 0) ? DrawCommand.SpriteTiles[0] : 1;
	DrawCommand.SpriteTiles[1] = (DrawCommand.SpriteTiles[1] > 0) ? DrawCommand.SpriteTiles[1] : 1;

	if (GDraw.SpriteCount < KMaxSpriteDrawCalls) {
		GDraw.SpriteQueue[GDraw.SpriteCount++] = DrawCommand;
	}
}

void DrawRender(void)
{
	for (int32 SpriteDrawIndex = 0; SpriteDrawIndex < GDraw.SpriteCount; SpriteDrawIndex++) {
		const SpriteDraw* DrawCommand = &GDraw.SpriteQueue[SpriteDrawIndex];

		if (DrawCommand->SpriteId <= 0) {
			continue;
		}

		SDL_Rect SourceRect = GetSpriteRect(
			&GDraw.SpriteSheets[0],
			DrawCommand->SpriteId,
			DrawCommand->SpriteTiles[0],
			DrawCommand->SpriteTiles[1]);

		SDL_FRect DestRect = {
			.x = DrawCommand->Position[0],
			.y = DrawCommand->Position[1],
			.w = SourceRect.w,
			.h = SourceRect.h,
		};

		// for now it's all in the first sprite sheet
		SDL_RenderCopyExF(
			GDraw.Renderer,
			GDraw.SpriteSheetTextures[0],
			&SourceRect,
			&DestRect,
			0.0,
			NULL,
			SDL_FLIP_NONE);
	}

	GDraw.SpriteCount = 0;
}

// Private Implementations

static SDL_Rect
GetSpriteRect(const SpriteSheet* spriteSheet, int32 spriteId, int32 spriteTilesX, int32 spriteTilesY)
{
	int32 SpriteTileX = spriteId % spriteSheet->SpritesPerRow;
	int32 SpriteTileY = spriteId / spriteSheet->SpritesPerRow;

	if (SpriteTileX + spriteTilesX > spriteSheet->SpritesPerRow)
		spriteTilesX = spriteSheet->SpritesPerRow - SpriteTileX;

	if (SpriteTileY + spriteTilesY > spriteSheet->SpritesPerCol)
		spriteTilesY = spriteSheet->SpritesPerCol - SpriteTileY;

	SDL_Rect Result = {
		.x = SpriteTileX * spriteSheet->SpriteWidth,
		.y = SpriteTileY * spriteSheet->SpriteHeight,
		.w = spriteTilesX * spriteSheet->SpriteWidth,
		.h = spriteTilesY * spriteSheet->SpriteHeight,
	};

	return Result;
}