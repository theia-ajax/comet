#include "Assets.h"

#include <SDL2/SDL.h>
#include <stb_ds.h>
#include <stb_image.h>

#include "JsonHelpers.h"
#include "Log.h"

// Constants

const char* AssetTypeNames[] = {
	"None",
	"Image",
	"SpriteSheetData",
};
_Static_assert(ARRAY_COUNT(AssetTypeNames) == AssetType_Count, "");

// Private Definitions

typedef int32 FreeStack;

struct {
	AssetTypeConfig TypeInfoDatabase[AssetType_Count];
	Asset* AssetStorage;
	FreeStack* AssetStorageFreeStack;
} GAssets;

// Private Prototypes

static int32 _FreeStackPop(FreeStack* Stack);
static Asset* _AcquireAsset(void);
static void _ReleaseAsset(Asset* AssetToRelease);

// Public Implementations

void AssetsInitialize(const AssetsConfig* config)
{
	ASSERT(config);

	memcpy(GAssets.TypeInfoDatabase, config->TypeConfigs, sizeof(GAssets.TypeInfoDatabase));

	for (int32 AssetIndex = AssetType_First; AssetIndex < AssetType_Count; AssetIndex++) {
		GAssets.TypeInfoDatabase[AssetIndex].Type = (AssetType)AssetIndex;
		ASSERT(GAssets.TypeInfoDatabase[AssetIndex].LoadAssetData);
		ASSERT(GAssets.TypeInfoDatabase[AssetIndex].UnloadAssetData);
	}

	arrsetcap(GAssets.AssetStorage, 256);
	arrsetcap(GAssets.AssetStorageFreeStack, 256);

	LogInfo(__FUNCTION__);
}

void AssetsShutdown(void)
{
	for (ptrdiff_t AssetIndex = arrlen(GAssets.AssetStorage) - 1; AssetIndex >= 0; AssetIndex--) {
		Asset* NextAsset = &GAssets.AssetStorage[AssetIndex];
		UnloadAsset(NextAsset);
	}

	arrfree(GAssets.AssetStorage);
	arrfree(GAssets.AssetStorageFreeStack);

	LogInfo(__FUNCTION__);
}

Asset* LoadAsset(AssetType Type, const char* FileName)
{
	Asset* Result = _AcquireAsset();

	const AssetTypeConfig* TypeInfo = &GAssets.TypeInfoDatabase[Type];

	Result->Meta.Type = Type;
	Result->Meta.Path = GetStringId(FileName);
	Result->Meta.Size = TypeInfo->Size;

	void* DataStorage = malloc(Result->Meta.Size);
	ASSERT(DataStorage != NULL);

	if (TypeInfo->LoadAssetData(FileName, DataStorage)) {
		Result->Data = DataStorage;
		LogInfo("Loaded asset '%s' with type '%s'", FileName, AssetTypeNames[Type]);
	} else {
		free(DataStorage);
		_ReleaseAsset(Result);
		Result = NULL;
		LogError("Failed to load asset '%s'", FileName);
	}

	return Result;
}

void UnloadAsset(Asset* AssetToUnload)
{
	if (!AssetToUnload) {
		return;
	}

	const AssetTypeConfig* TypeInfo = &GAssets.TypeInfoDatabase[AssetToUnload->Meta.Type];
	TypeInfo->UnloadAssetData(AssetToUnload->Data);
	free(AssetToUnload->Data);
	_ReleaseAsset(AssetToUnload);
}

// Private Implementations

static int32 _FreeStackPop(FreeStack* Stack)
{
	int32 Result = NONE;
	if (arrlen(Stack) > 0) {
		Result = arrpop(Stack);
	}
	return Result;
}

// Just acquires memory for the asset, does not set any fields or load any data
static Asset* _AcquireAsset(void)
{
	Asset* Result = NULL;
	int32 Index = _FreeStackPop(GAssets.AssetStorageFreeStack);
	if (Index != NONE) {
		Result = &GAssets.AssetStorage[Index];
	} else {
		arrput(GAssets.AssetStorage, (Asset){0});
		Result = arrlastp(GAssets.AssetStorage);
	}
	return Result;
}

// Assumes data has already been freed
static void _ReleaseAsset(Asset* AssetToRelease)
{
	ptrdiff_t IndexOf = AssetToRelease - GAssets.AssetStorage;
	ASSERT(VALID_INDEX(IndexOf, arrlenu(GAssets.AssetStorage)));
	arrput(GAssets.AssetStorageFreeStack, IndexOf);
	ZERO_STRUCT(&GAssets.AssetStorage[IndexOf]);
}
