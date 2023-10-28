#pragma once

#include "Types.h"

typedef enum AssetTypeId {
	AssetTypeId_Invalid,
	AssetTypeId_Texture,
	AssetTypeId_Count,
} AssetTypeId;

typedef void* (*LoadAssetFunc)(const char* FileName);
typedef void (*UnloadAssetFunc)(void* Asset);

void AssetDatabase_Initialize(void);
void AssetDatabase_Shutdown(void);
void AssetDatabase_RegisterAssetType(
	AssetTypeId AssetType,
	LoadAssetFunc LoadAsset,
	UnloadAssetFunc UnloadAsset);
void* AssetDatabase_LoadAssetWithType(AssetTypeId AssetType, const char* AssetName);