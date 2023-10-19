#pragma once

#include "types.h"

typedef struct SDL_Surface SDL_Surface;

typedef struct AssetsConfig
{

} AssetsConfig;

typedef struct ImageAsset
{
	uint8 *Pixels;
	SDL_Surface *Surface;
	int32 Width;
	int32 Height;
	int32 Pitch;
} ImageAsset;

void AssetsInitialize(const AssetsConfig *config);
void AssetsShutdown(void);

ImageAsset *LoadImageAsset(const char *fileName);
void UnloadImageAsset(ImageAsset *image);