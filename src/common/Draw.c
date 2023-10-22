#include "Draw.h"

#include <SDL2/SDL.h>
#include <json.h>

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

void DrawRender(void)
{
	for (int32 SpriteDrawIndex = 0; SpriteDrawIndex < GDraw.SpriteCount; SpriteDrawIndex++) {
		const SpriteDraw* DrawCommand = &GDraw.SpriteQueue[SpriteDrawIndex];

		SDL_Rect SourceRect = GetSpriteRect(
			&GDraw.SpriteSheets[DrawCommand->SpriteSheetId],
			DrawCommand->SpriteId,
			DrawCommand->SpriteTiles[0],
			DrawCommand->SpriteTiles[1]);

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

		// for now it's all in the first sprite sheet
		SDL_RenderCopyExF(
			GDraw.Renderer,
			GDraw.SpriteSheetTextures[DrawCommand->SpriteSheetId],
			&SourceRect,
			&DestRect,
			DrawCommand->Rotation * TurnToDeg,
			&Center,
			SDL_FLIP_NONE);

		// SDL_FRect PosRect = (SDL_FRect){
		// 	.x = DestRect.x + Center.x,
		// 	.y = DestRect.y + Center.y,
		// 	.w = 1.0f,
		// 	.h = 1.0f,
		// };
		// SDL_SetRenderDrawColor(GDraw.Renderer, 0, 255, 255, 0);
		// SDL_RenderDrawRectF(GDraw.Renderer, &PosRect);
	}

	GDraw.SpriteCount = 0;
}

// Private Implementations

static SDL_Rect GetSpriteRect(
	const SpriteSheet* spriteSheet,
	int32 spriteId,
	int32 spriteTilesX,
	int32 spriteTilesY)
{
	SDL_Rect Result;

	switch (spriteSheet->SheetType) {
		case SpriteSheetType_Grid:
			{
				int32 SpriteTileX = spriteId % spriteSheet->SpritesPerRow;
				int32 SpriteTileY = spriteId / spriteSheet->SpritesPerRow;

				if (SpriteTileX + spriteTilesX > spriteSheet->SpritesPerRow)
					spriteTilesX = spriteSheet->SpritesPerRow - SpriteTileX;

				if (SpriteTileY + spriteTilesY > spriteSheet->SpritesPerCol)
					spriteTilesY = spriteSheet->SpritesPerCol - SpriteTileY;

				Result = (SDL_Rect){
					.x = SpriteTileX * spriteSheet->SpriteWidth,
					.y = SpriteTileY * spriteSheet->SpriteHeight,
					.w = spriteTilesX * spriteSheet->SpriteWidth,
					.h = spriteTilesY * spriteSheet->SpriteHeight,
				};
			}
			break;
		case SpriteSheetType_Frames:
			{
				Rect16 R = spriteSheet->SheetData->Data->Frames.Frame[spriteId];
				Result = (SDL_Rect) {
					R.X, R.Y, R.W, R.H
				};
			}
			break;
		default:
			break;
	}

	return Result;
}