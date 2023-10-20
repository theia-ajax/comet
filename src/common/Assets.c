#include "Assets.h"

#include <SDL2/SDL.h>
#include <stb_ds.h>
#include <stb_image.h>

// Private Definitions

typedef int32 FreeStack;

struct {
	AssetsConfig Config;
	ImageAsset* ImageAssets;
	FreeStack* ImageAssetsFreeStack;
} GAssets;

// Private Prototypes

static ImageAsset* AllocateImageAsset();
static void ReleaseImageAsset(ImageAsset* image);
static void FreeImageAssetResources(ImageAsset* image);

// Public Implementations

void AssetsInitialize(const AssetsConfig* config)
{
	arrsetcap(GAssets.ImageAssets, 256);
	arrsetcap(GAssets.ImageAssetsFreeStack, 256);
}

void AssetsShutdown(void)
{
	for (size_t ImageIndex = 0; ImageIndex < arrlenu(GAssets.ImageAssets); ImageIndex++) {
		FreeImageAssetResources(&GAssets.ImageAssets[ImageIndex]);
	}

	arrfree(GAssets.ImageAssets);
	arrfree(GAssets.ImageAssetsFreeStack);
}

ImageAsset* LoadImageAsset(const char* fileName)
{
	ImageAsset* Result = AllocateImageAsset();

	// Hardcoding 4 bytes per pixel regardless of source because lazy

	int Width, Height, Channels;
	Result->Pixels = stbi_load(fileName, &Width, &Height, &Channels, 4);

	if (Result->Pixels == NULL) {
		ReleaseImageAsset(Result);
		return NULL;
	}

	Result->Width = (int32)Width;
	Result->Height = (int32)Height;
	Result->Pitch = Result->Width * 4;

	Result->Surface = SDL_CreateRGBSurfaceWithFormatFrom(
		Result->Pixels, Result->Width, Result->Height, 32, Result->Pitch, SDL_PIXELFORMAT_ABGR8888);

	return Result;
}

void UnloadImageAsset(ImageAsset* image)
{
	FreeImageAssetResources(image);
	ReleaseImageAsset(image);
}

// Private Implementations

static ImageAsset* AllocateImageAsset()
{
	if (arrlen(GAssets.ImageAssetsFreeStack) > 0) {
		int32 Index = arrpop(GAssets.ImageAssetsFreeStack);
		return &GAssets.ImageAssets[Index];
	}

	arrput(GAssets.ImageAssets, (ImageAsset){0});
	return arrlastp(GAssets.ImageAssets);
}

static void ReleaseImageAsset(ImageAsset* image)
{
	ASSERT(image != NULL);

	ptrdiff_t IndexOf = image - GAssets.ImageAssets;
	ASSERT(VALID_INDEX(IndexOf, arrlen(GAssets.ImageAssets)));

	arrput(GAssets.ImageAssetsFreeStack, IndexOf);
}

static void FreeImageAssetResources(ImageAsset* image)
{
	SDL_FreeSurface(image->Surface);
	stbi_image_free(image->Pixels);
	ZERO_STRUCT(image);
}
