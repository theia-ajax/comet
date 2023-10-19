#include "AssetDatabase.h"

#include <stb_ds.h>

typedef struct AssetNode {
	const char *Key;
	void *Value;
} AssetNode;

typedef struct TypedAssetDatabase {
	AssetNode *AssetMap;
	LoadAssetFunc LoadAsset;
	UnloadAssetFunc UnloadAsset;
} TypedAssetDatabase;

typedef struct TypedAssetDatabaseNode {
	AssetTypeId Key;
	TypedAssetDatabase Value;
} TypedAssetDatabaseNode;

struct {
	TypedAssetDatabaseNode *TypedAssetDatabaseMap;
} GAssetDatabase;

const TypedAssetDatabase KDefaultTypedAssetDatabase = {0};

void AssetDatabase_Initialize(void)
{
	for (AssetTypeId typeId = 0; typeId < AssetTypeId_Count; typeId++) {
		stbds_hmdefault(GAssetDatabase.TypedAssetDatabaseMap, KDefaultTypedAssetDatabase);
	}
}

void AssetDatabase_Shutdown(void)
{
	hmfree(GAssetDatabase.TypedAssetDatabaseMap);
}

void AssetDatabase_RegisterAssetType(
	AssetTypeId AssetType,
	LoadAssetFunc LoadAsset,
	UnloadAssetFunc UnloadAsset)
{
	ASSERT(
		hmgeti(GAssetDatabase.TypedAssetDatabaseMap, AssetType) < 0
		&& "AssetType already registered.");

	hmput(
		GAssetDatabase.TypedAssetDatabaseMap,
		AssetType,
		((TypedAssetDatabase){
			.LoadAsset = LoadAsset,
			.UnloadAsset = UnloadAsset,
		}));
}

void *AssetDatabase_LoadAssetWithType(AssetTypeId AssetType, const char *AssetName)
{
	void *Result = NULL;
	TypedAssetDatabase *AssetDatabase =
		&hmgetp(GAssetDatabase.TypedAssetDatabaseMap, AssetType)->Value;

	ASSERT(AssetDatabase != NULL && "AssetType not registered.");

	if (shgeti(AssetDatabase->AssetMap, AssetName) >= 0) {
		Result = shget(AssetDatabase->AssetMap, AssetName);
	} else {
		Result = AssetDatabase->LoadAsset(AssetName);

		if (Result != NULL) {
			shput(AssetDatabase->AssetMap, AssetName, Result);
		}
	}

	return Result;
}
