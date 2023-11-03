#pragma once

#include "Math2D.h"
#include "StringId.h"
#include "Types.h"

typedef struct SDL_Surface SDL_Surface;

typedef enum AssetType {
	AssetType_None,
	AssetType_Image,
	AssetType_SpriteSheetData,
	AssetType_Count,

	AssetType_First = AssetType_Image,
} AssetType;

typedef bool (*LoadAssetDataFunc)(const char* FileName, void* DataOut);
typedef void (*UnloadAssetDataFunc)(void* Data);

typedef struct AssetTypeConfig {
	AssetType Type;
	size_t Size;
	LoadAssetDataFunc LoadAssetData;
	UnloadAssetDataFunc UnloadAssetData;
} AssetTypeConfig;

typedef struct AssetsConfig {
	AssetTypeConfig TypeConfigs[AssetType_Count];
} AssetsConfig;

typedef struct AssetMetaData {
	AssetType Type;
	StringId Path;
	size_t Size;
} AssetMetaData;

typedef struct Asset {
	AssetMetaData Meta;
	void* Data;
} Asset;

#define ASSET(Type) CAT(Type, Asset)
#define ASSET_DATA(Type) CAT(Type, Data)
#define ASSET_CAST(Type, Asset) (ASSET(Type)*)(Asset)

#define DEFINE_ASSET(Type)                                                                     \
	typedef struct ASSET(Type) {                                                               \
		AssetMetaData Meta;                                                                        \
		ASSET_DATA(Type)* Data;                                                                            \
	} ASSET(Type);

AssetType GetAssetType(const Asset* Self);
const char* GetAssetPath(const Asset* Self);

void AssetsInitialize(const AssetsConfig* config);
void AssetsShutdown(void);

Asset* LoadAsset(AssetType Type, const char* FileName);
void UnloadAsset(Asset* AssetToUnload);
