#include "Assets.h"

#include <SDL3/SDL.h>
#include <stb_ds.h>
#include <stb_image.h>

#include "JsonHelpers.h"
#include "Log.h"

// // Constants

const char* AssetTypeNames[] = {
	"None",
	"Image",
	"SpriteSheet",
};
_Static_assert(ARRAY_COUNT(AssetTypeNames) == AssetType_Count, "");

// Private Definitions

// typedef int32 FreeStack;

// struct {
// 	AssetTypeConfig TypeInfoDatabase[AssetType_Count];
// 	Asset* AssetStorage;
// 	FreeStack* AssetStorageFreeStack;
// } GAssets;

// // Private Prototypes

// static int32 _FreeStackPop(FreeStack* Stack);
// static Asset* _AcquireAsset(void);
// static void _ReleaseAsset(Asset* AssetToRelease);

// // Public Implementations

// void AssetsInitialize(const AssetsConfig* config)
// {
// 	ASSERT(config);

// 	memcpy(GAssets.TypeInfoDatabase, config->TypeConfigs, sizeof(GAssets.TypeInfoDatabase));

// 	for (int32 AssetIndex = AssetType_First; AssetIndex < AssetType_Count; AssetIndex++) {
// 		GAssets.TypeInfoDatabase[AssetIndex].Type = (AssetType)AssetIndex;
// 		ASSERT(GAssets.TypeInfoDatabase[AssetIndex].LoadAssetData);
// 		ASSERT(GAssets.TypeInfoDatabase[AssetIndex].UnloadAssetData);
// 	}

// 	arrsetcap(GAssets.AssetStorage, 256);
// 	arrsetcap(GAssets.AssetStorageFreeStack, 256);

// 	LogInfo(__FUNCTION__);
// }

// void AssetsShutdown(void)
// {
// 	for (ptrdiff_t AssetIndex = arrlen(GAssets.AssetStorage) - 1; AssetIndex >= 0; AssetIndex--) {
// 		Asset* NextAsset = &GAssets.AssetStorage[AssetIndex];
// 		UnloadAsset(NextAsset);
// 	}

// 	arrfree(GAssets.AssetStorage);
// 	arrfree(GAssets.AssetStorageFreeStack);

// 	LogInfo(__FUNCTION__);
// }

// Asset* LoadAsset(AssetType Type, const char* FileName)
// {
// 	Asset* Result = _AcquireAsset();

// 	const AssetTypeConfig* TypeInfo = &GAssets.TypeInfoDatabase[Type];

// 	Result->Meta.Type = Type;
// 	Result->Meta.Path = GetStringId(FileName);
// 	Result->Meta.Size = TypeInfo->Size;

// 	void* DataStorage = malloc(Result->Meta.Size);
// 	ASSERT(DataStorage != NULL);

// 	if (TypeInfo->LoadAssetData(FileName, DataStorage)) {
// 		Result->Data = DataStorage;
// 		LogInfo("Loaded asset '%s' with type '%s'", FileName, AssetTypeNames[Type]);
// 	} else {
// 		free(DataStorage);
// 		_ReleaseAsset(Result);
// 		Result = NULL;
// 		LogError("Failed to load asset '%s'", FileName);
// 	}

// 	return Result;
// }

// void UnloadAsset(Asset* AssetToUnload)
// {
// 	if (!AssetToUnload) {
// 		return;
// 	}

// 	const AssetTypeConfig* TypeInfo = &GAssets.TypeInfoDatabase[AssetToUnload->Meta.Type];
// 	TypeInfo->UnloadAssetData(AssetToUnload->Data);
// 	free(AssetToUnload->Data);
// 	_ReleaseAsset(AssetToUnload);
// }

// // Private Implementations

// static int32 _FreeStackPop(FreeStack* Stack)
// {
// 	int32 Result = NONE;
// 	if (arrlen(Stack) > 0) {
// 		Result = arrpop(Stack);
// 	}
// 	return Result;
// }

// // Just acquires memory for the asset, does not set any fields or load any data
// static Asset* _AcquireAsset(void)
// {
// 	Asset* Result = NULL;
// 	int32 Index = _FreeStackPop(GAssets.AssetStorageFreeStack);
// 	if (Index != NONE) {
// 		Result = &GAssets.AssetStorage[Index];
// 	} else {
// 		arrput(GAssets.AssetStorage, (Asset){0});
// 		Result = arrlastp(GAssets.AssetStorage);
// 	}
// 	return Result;
// }

// // Assumes data has already been freed
// static void _ReleaseAsset(Asset* AssetToRelease)
// {
// 	ptrdiff_t IndexOf = AssetToRelease - GAssets.AssetStorage;
// 	ASSERT(VALID_INDEX(IndexOf, arrlenu(GAssets.AssetStorage)));
// 	arrput(GAssets.AssetStorageFreeStack, IndexOf);
// 	ZERO_STRUCT(&GAssets.AssetStorage[IndexOf]);
// }

typedef struct Asset {
	AssetType Type;
	StringId PathId;
	void* AssetData;
	char* FileData;
	size_t FileSize;
	int64 LastModTime;
	bool NeedsReimport;
} Asset;

typedef struct AssetIdMap {
	StringId Key;
	Asset* Value;
} AssetIdMap;

AssetIdMap* AssetMap = NULL;

Asset* _InternalAssetAlloc(size_t SpaceNeeded)
{
	size_t FullSize = sizeof(Asset) + SpaceNeeded;
	Asset* Result = (Asset*)malloc(FullSize);
	Result->AssetData = (uint8*)Result + sizeof(Asset);
	memset(Result, 0, FullSize);
	return Result;
}

void AssetsInitialize(void)
{
	hmdefault(AssetMap, NULL);
}

void AssetsShutdown(void)
{
	hmfree(AssetMap);
}

Asset* LoadAsset(const char* FileName, size_t AssetSize)
{
	StringId PathId = GetStringId(FileName);

	Asset* Result = hmget(AssetMap, PathId);
	if (Result != NULL) {
		return Result;
	}

	Result = _InternalAssetAlloc(AssetSize);

	SDL_RWops* File = SDL_RWFromFile(FileName, "r");
	if (File != NULL) {
		Result->FileSize = SDL_RWsize(File);
		if (Result->FileSize != NONE) {
			Result->FileData = (char*)malloc(Result->FileSize + 1);
			if (SDL_RWread(File, Result->AssetData, Result->FileSize) == Result->FileSize) {
				Result->NeedsReimport = true;
			} else {
				free(Result->FileData);
				Result->FileData = NULL;
				Result->FileSize = 0;
				LogError("Assets:LoadAsset: Unable to read file contents from '%s'.", FileName);
			}
		}
	}
	SDL_RWclose(File);

	return Result;
}

// Asset* LoadAsset(const char* FileName, )

typedef struct Image {
	uint8* Pixels;
	SDL_Surface* Surface;
} Image;

static inline Asset* _InternalGetAsset(void* AssetData)
{
	return (AssetData != NULL) ? (Asset*)AssetData - 1 : NULL;
}

bool ImportImage(const void* Memory, size_t Size, Image* OutImage)
{
	Asset* ImageAsset = _InternalGetAsset(OutImage);

	ZERO_STRUCT(OutImage);

	int Width, Height, Channels;
	uint8* Pixels = stbi_load_from_memory(Memory, Size, &Width, &Height, &Channels, 4);

	if (Pixels != NULL) {

		SDL_Surface* Surface = SDL_CreateSurfaceFrom(Pixels, Width, Height, Width * 4, SDL_PIXELFORMAT_ARGB8888);

		ASSERT(Surface != NULL);

		if (Surface != NULL) {
			OutImage->Pixels = Pixels;
			OutImage->Surface = Surface;
		} else {
			stbi_image_free(Pixels);
			LogError("Assets:ImportImage: Unable to create surface from '%s'.", StringIdCStr(ImageAsset->PathId));
		}
	} else {
		LogError("Assets:ImportImage: Unable to import image from '%s'.", StringIdCStr(ImageAsset->PathId));
	}

	return OutImage->Pixels != NULL;
}

Image* LoadImageAsset(const char* FileName)
{
	StringId PathId = GetStringId(FileName);
	Asset* Asset = LoadAsset(FileName, sizeof(Image));
	Image* Result = (Image*)(Asset->AssetData);
	if (Asset->NeedsReimport) {
		if (ImportImage(Asset->FileData, Asset->FileSize, Result)) {
			LogInfo("Assets:LoadImageAsset: Loaded image at '%s'.", FileName);
		} else {
			Result = NULL;
			LogError("Assets:LoadImageAsset: Failed to load image at '%s'.", FileName);
		}
	}
	return Result;
}
