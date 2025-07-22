#include "SpriteDatabase.h"

#include <SDL3/SDL.h>
#include <stb_ds.h>

// private declarations

enum {
	KMaxSpriteSheets = 128,
};

struct {
	SDL_Renderer* Renderer;
	FixedList(SpriteSheet, KMaxSpriteSheets) SpriteSheets;
} GSpriteDatabase;

// private interface

static inline SpriteId BuildSpriteId(int32 SheetIndex, int32 SpriteIndex);
static inline void SplitSpriteId(SpriteId SpriteHandle, int32* OutSheetIndex, int32* OutSpriteIndex);

// public implementation

void SpriteDatabaseInitialize(SDL_Renderer* Renderer)
{
	GSpriteDatabase.Renderer = Renderer;
}

void SpriteDatabaseShutdown(void)
{
}

SpriteSheetId SpriteDatabaseCreateGridSpriteSheet(
	StringId Name,
	ImageAsset* Image,
	int32 SpriteWidth,
	int32 SpriteHeight)
{
	ASSERT(Image != NULL);
	ASSERT(SpriteWidth > 0);
	ASSERT(SpriteHeight > 0);
	ASSERT(!FixedListIsFull(GSpriteDatabase.SpriteSheets));

	int32 SheetIndex = GSpriteDatabase.SpriteSheets.Count;
	SpriteSheet* NewSheet = FixedListPush(GSpriteDatabase.SpriteSheets);

	*NewSheet = (SpriteSheet){
		.Name = Name,
		.SheetType = SpriteSheetType_Grid,
		.Image = Image,
		.SpriteWidth = SpriteWidth,
		.SpriteHeight = SpriteHeight,
		.Texture = SDL_CreateTextureFromSurface(GSpriteDatabase.Renderer, Image->Data->Surface),
	};

	SDL_SetTextureScaleMode(NewSheet->Texture, SDL_SCALEMODE_NEAREST);

	NewSheet->SpritesPerRow = NewSheet->Image->Data->Surface->w / SpriteWidth;
	NewSheet->SpritesPerCol = NewSheet->Image->Data->Surface->h / SpriteHeight;

	return (SpriteSheetId){SheetIndex};
}

SpriteSheetId SpriteDatabaseCreateFrameDataSpriteSheet(StringId Name, ImageAsset* Image, SpriteSheetAsset* Sheet)
{
	ASSERT(Image != NULL);
	ASSERT(Sheet != NULL);
	ASSERT(!FixedListIsFull(GSpriteDatabase.SpriteSheets));

	int32 SheetIndex = GSpriteDatabase.SpriteSheets.Count;
	SpriteSheet* NewSheet = FixedListPush(GSpriteDatabase.SpriteSheets);

	*NewSheet = (SpriteSheet){
		.Name = Name,
		.SheetType = SpriteSheetType_Frames,
		.Image = Image,
		.SheetData = Sheet,
		.Texture = SDL_CreateTextureFromSurface(GSpriteDatabase.Renderer, Image->Data->Surface),
	};

	SDL_SetTextureScaleMode(NewSheet->Texture, SDL_SCALEMODE_NEAREST);

	return (SpriteSheetId){SheetIndex};
}

const SpriteSheet* SpriteDatabaseTryGetSpriteSheet(SpriteSheetId SpriteSheetHandle)
{
	const SpriteSheet* Result = NULL;
	if (VALID_INDEX(SpriteSheetHandle.Value, FixedListCapacity(GSpriteDatabase.SpriteSheets))) {
		const SpriteSheet* Sheet = FixedListAt(GSpriteDatabase.SpriteSheets, SpriteSheetHandle.Value);
		if (Sheet->SheetType != SpriteSheetType_None) {
			Result = Sheet;
		}
	}
	return Result;
}

const SpriteSheet* SpriteDatabaseGetSpriteSheet(SpriteSheetId SpriteSheetHandle)
{
	ASSERT(SpriteSheetHandle.Value != NONE);
	ASSERT(VALID_INDEX(SpriteSheetHandle.Value, FixedListCapacity(GSpriteDatabase.SpriteSheets)));
	ASSERT(FixedListAt(GSpriteDatabase.SpriteSheets, SpriteSheetHandle.Value)->SheetType != SpriteSheetType_None);
	return FixedListAt(GSpriteDatabase.SpriteSheets, SpriteSheetHandle.Value);
}

int32 SpriteSheetSpriteCount(SpriteSheetId SpriteSheetHandle)
{
	ASSERT(false);
	return 0;
}

SpriteId SpriteFindByName(const char* SpriteName)
{
	return SpriteFindByNameId(GetStringId(SpriteName));
}

SpriteId SpriteFindByNameId(StringId SpriteName)
{
	SpriteId Found = (SpriteId){NONE};

	for (int32 SheetIndex = 0; SheetIndex < GSpriteDatabase.SpriteSheets.Count; SheetIndex++) {

		const SpriteSheet* Sheet = FixedListAt(GSpriteDatabase.SpriteSheets, SheetIndex);

		if (Sheet->SheetType == SpriteSheetType_Frames && Sheet->SheetData != NULL) {
			int32 SpriteIndex = hmget(Sheet->SheetData->Data->NameIdMap, SpriteName);
			if (SpriteIndex != NONE) {
				Found = BuildSpriteId(SheetIndex, SpriteIndex);
				break;
			}
		}
	}

	return Found;
}

SpriteId SpriteSheetFindSpriteByName(SpriteSheetId SpriteSheetHandle, const char* SpriteName)
{
	return SpriteSheetFindSpriteByNameId(SpriteSheetHandle, GetStringId(SpriteName));
}

SpriteId SpriteSheetFindSpriteByNameId(SpriteSheetId SpriteSheetHandle, StringId SpriteName)
{
	SpriteId Found = (SpriteId){NONE};

	if (VALID_HANDLE(SpriteSheetHandle) && VALID_INDEX(SpriteSheetHandle.Value, GSpriteDatabase.SpriteSheets.Count)) {
		const SpriteSheet* Sheet = FixedListAt(GSpriteDatabase.SpriteSheets, SpriteSheetHandle.Value);

		if (Sheet->SheetType == SpriteSheetType_Frames && Sheet->SheetData != NULL) {
			int32 SpriteIndex = hmget(Sheet->SheetData->Data->NameIdMap, SpriteName);
			if (SpriteIndex != NONE) {
				Found = BuildSpriteId(SpriteSheetHandle.Value, SpriteIndex);
			}
		}
	}

	return Found;
}

SpriteId SpriteSheetFindSpriteByIndex(SpriteSheetId SpriteSheetHandle, int32 SpriteIndex)
{
	SpriteId Result = (SpriteId){NONE};
	const SpriteSheet* Sheet = SpriteDatabaseTryGetSpriteSheet(SpriteSheetHandle);
	if (Sheet != NULL && SpriteIndex >= 0 && SpriteIndex < Sheet->SpritesPerCol * Sheet->SpritesPerRow) {
		Result = BuildSpriteId(SpriteSheetHandle.Value, SpriteIndex);
	}
	return Result;
}

SpriteSheetId SpriteSheetFindByName(StringId Name)
{
	SpriteSheetId Found = (SpriteSheetId){NONE};
	for (int32 Index = 0; Index < GSpriteDatabase.SpriteSheets.Count; Index++) {
		if (StringIdEq(GSpriteDatabase.SpriteSheets.Data[Index].Name, Name)) {
			Found = (SpriteSheetId){Index};
		}
	}
	return Found;
}

SDL_Texture* GetSpriteTexture(SpriteId SpriteHandle)
{
	SDL_Texture* Result = NULL;
	if (VALID_HANDLE(SpriteHandle)) {
		int32 SheetIndex, SpriteIndex;
		SplitSpriteId(SpriteHandle, &SheetIndex, &SpriteIndex);

		ASSERT(VALID_INDEX(SheetIndex, GSpriteDatabase.SpriteSheets.Count));
		Result = FixedListAt(GSpriteDatabase.SpriteSheets, SheetIndex)->Texture;
	}
	return Result;
}

SpriteRect GetSpriteRect(SpriteId SpriteHandle)
{
	SpriteRect Result;
	ZERO_STRUCT(&Result);

	if (VALID_HANDLE(SpriteHandle)) {
		int32 SheetIndex, SpriteIndex;
		SplitSpriteId(SpriteHandle, &SheetIndex, &SpriteIndex);

		ASSERT(VALID_INDEX(SheetIndex, GSpriteDatabase.SpriteSheets.Count));

		const SpriteSheet* Sheet = FixedListAt(GSpriteDatabase.SpriteSheets, SheetIndex);

		switch (Sheet->SheetType) {
			case SpriteSheetType_Frames:
				ASSERT(VALID_INDEX(SpriteIndex, Sheet->SheetData->Data->Frames.Count));
				Rect16 FrameRect = Sheet->SheetData->Data->Frames.Frame[SpriteIndex];
				Result = (SpriteRect){FrameRect.X, FrameRect.Y, FrameRect.W, FrameRect.H};
				break;

			case SpriteSheetType_Grid:
				ASSERT(VALID_INDEX(SpriteIndex, Sheet->SpritesPerRow * Sheet->SpritesPerCol));
				int32 SpriteTileX = SpriteIndex % Sheet->SpritesPerRow;
				int32 SpriteTileY = SpriteIndex / Sheet->SpritesPerRow;

				Result = (SpriteRect){
					.X = SpriteTileX * Sheet->SpriteWidth,
					.Y = SpriteTileY * Sheet->SpriteHeight,
					.W = Sheet->SpriteWidth,
					.H = Sheet->SpriteHeight,
				};
				break;

			default: unreachable(); break;
		}
	}
	return Result;
}

// private implementations

static inline SpriteId BuildSpriteId(int32 SheetIndex, int32 SpriteIndex)
{
	ASSERT(SheetIndex >= 0);
	ASSERT(SpriteIndex >= 0);
	return (SpriteId){(SheetIndex & 0x7F) << 24 | SpriteIndex};
}

static inline void SplitSpriteId(SpriteId SpriteHandle, int32* OutSheetIndex, int32* OutSpriteIndex)
{
	ASSERT(VALID_HANDLE(SpriteHandle));
	ASSERT(OutSheetIndex != NULL);
	ASSERT(OutSpriteIndex != NULL);
	*OutSpriteIndex = SpriteHandle.Value & 0xFFFFFF;
	*OutSheetIndex = (SpriteHandle.Value >> 24) & 0x7F;
}
