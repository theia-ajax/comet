#include "GameWorld.h"

#include "stb_ds.h"

#include "Log.h"
#include "StringId.h"
#include "Util.h"

// clang-format off
#define KEntityIndexBits 16
#define KEntityIndexMask ((1 << (KEntityIndexBits - 1)) - 1)
#define ENTITY_ID(Index, Generation) (EntityId) { ((Index) & KEntityIndexMask) | ((Generation) << KEntityIndexBits) }
#define ENTITY_ID_INDEX(Entity) ((Entity.RawValue) & KEntityIndexMask)
#define ENTITY_ID_GENERATION(Entity) ((Entity.RawValue) >> KEntityIndexBits)
#define ENTITY_ID_INVALID (EntityId){0}
#define ENTITY_ID_EQ(A, B) ((A).RawValue == (B).RawValue)
#define ENTITY_ID_NEQ(A, B) ((A).RawValue != (B).RawValue)
// clang-format on


const int32 KInitialEntityCapacity = 32;
const struct {
	const char* Name;
	int32 Size;
	int32 Capacity;
} ComponentTypeData[] = {
	{"Transform", sizeof(TransformComponent), KInitialEntityCapacity},
	{"Sprite", sizeof(SpriteComponent), KInitialEntityCapacity},
	{"Collider", sizeof(ColliderComponent), KInitialEntityCapacity},
};

_Static_assert(
	ARRAY_COUNT(ComponentTypeData) == (sizeof(ComponentIdSet) / sizeof(StringId)),
	"ComponentIdSet missing component ID or ComponentTypeData missing type data");

#define REGISTER_COMPONENT_ID(Type) CID.Type = GetStringId(#Type);

static bool GStaticGameStateDataInitialized = false;
typedef struct ComponentIdSet {
	StringId Transform;
	StringId Sprite;
	StringId Collider;
} ComponentIdSet;
ComponentIdSet CID = {0};

typedef struct UntypedComponentList {
	StringId Key;
	int32 Count;
	int32 Capacity;
	int32 ComponentSize;
	void* ComponentMemory;
	int32* Indices;
	EntityId* Entities;
} UntypedComponentList;
#define ComponentListCast(List, Type) ((Type)*)((List).ComponentMemory)

UntypedComponentList CreateComponentList(StringId ComponentType, int32 ComponentSize, int32 Capacity)
{
	UntypedComponentList Self;
	ZERO_STRUCT(&Self);
	Self.Key = ComponentType;
	Self.ComponentSize = ComponentSize;
	Self.Capacity = Capacity;
	Self.ComponentMemory = malloc(Self.ComponentSize * Self.Capacity);
	ASSERT(Self.ComponentMemory);
	arrsetcap(Self.Indices, Self.Capacity);
	arrsetcap(Self.Entities, Self.Capacity);
	memset(Self.Indices, 0, sizeof(*Self.Indices) * arrcap(Self.Indices));
	memset(Self.Entities, 0, sizeof(*Self.Entities) * arrcap(Self.Entities));
	return Self;
}

static inline void* ComponentMemory(UntypedComponentList* Self, int32 Index)
{
	return ((uint8*)Self->ComponentMemory) + (Index * Self->ComponentSize);
}

void DestroyComponentList(UntypedComponentList* Self)
{
	free(Self->ComponentMemory);
	arrfree(Self->Indices);
	arrfree(Self->Entities);
	ZERO_STRUCT(Self);
}

void* ComponentListAdd(UntypedComponentList* List, EntityId Entity, const void* Component)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	int32 NewIndex = List->Count;

	ASSERT(EntityIndex <= arrlen(List->Indices) && "Entity has invalid index.");
	ASSERT(NewIndex <= arrlen(List->Entities));
	ASSERT(List->Indices[EntityIndex] == 0 && "Entity already has component");

	if (EntityIndex == arrlen(List->Indices)) {
		arrput(List->Indices, NewIndex);
	} else {
		List->Indices[EntityIndex] = NewIndex;
	}

	if (NewIndex == arrlen(List->Entities)) {
		arrput(List->Entities, Entity);
	} else {
		List->Entities[NewIndex] = Entity;
	}

	void* Storage = ((uint8*)List->ComponentMemory) + NewIndex * List->ComponentSize;
	if (Component != NULL) {
		memcpy(Storage, Component, List->ComponentSize);
	} else {
		memset(Storage, 0, List->ComponentSize);
	}
	List->Count++;
	return Storage;
}

void ComponentListRemove(UntypedComponentList* Self, EntityId Entity)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	ASSERT(Self->Indices[EntityIndex] != 0);

	int32 RemovedIndex = Self->Indices[EntityIndex];
	int32 LastIndex = Self->Count - 1;

	void* RemovedMemory = ComponentMemory(Self, RemovedIndex);
	void* LastMemory = ComponentMemory(Self, Self->Count - 1);
	memcpy(RemovedMemory, LastMemory, Self->ComponentSize);
	memset(LastMemory, 0, Self->ComponentSize);
	Self->Count--;

	EntityId LastEntity = Self->Entities[LastIndex];
	int32 LastEntityIndex = ENTITY_ID_INDEX(LastEntity);
	Self->Indices[LastEntityIndex] = RemovedIndex;
	Self->Entities[RemovedIndex] = LastEntity;

	Self->Indices[EntityIndex] = NONE;
	Self->Entities[LastIndex] = ENTITY_ID_INVALID;
}

void* ComponentListGet(UntypedComponentList* Self, EntityId Entity)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	ASSERT(ENTITY_ID_NEQ(Self->Entities[EntityIndex], ENTITY_ID_INVALID));

	int32 ComponentIndex = Self->Indices[EntityIndex];
	void* Result = ComponentMemory(Self, ComponentIndex);
	return Result;
}

bool ComponentListHas(UntypedComponentList* Self, EntityId Entity)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	return Self->Indices[EntityIndex] != NONE;
}


static void InitializeStaticData(void)
{
	GStaticGameStateDataInitialized = true;

	REGISTER_COMPONENT_ID(Transform);
	REGISTER_COMPONENT_ID(Sprite);
	REGISTER_COMPONENT_ID(Collider);
}

typedef struct GameWorld {
	// GameEntity* Entities;
	EntityId* ActiveEntities;
	int32* EntityGenerations;
	int32* AvailableIndexStack;

	UntypedComponentList* ComponentLists;

	// New band/album name
	int32 AllTimeHighGeneration;
	int32 AllTimeHighGenerationFirstIndex;
} GameWorld;

GameWorld* CreateGameWorld(void)
{
	if (!GStaticGameStateDataInitialized) {
		InitializeStaticData();
	}

	GameWorld* NewState = (GameWorld*)malloc(sizeof(GameWorld));
	ZERO_STRUCT(NewState);

	// arrsetcap(NewState->Entities, KInitialEntityCapacity);
	arrsetcap(NewState->ActiveEntities, KInitialEntityCapacity);
	arrsetcap(NewState->EntityGenerations, KInitialEntityCapacity);
	arrsetcap(NewState->AvailableIndexStack, 32);

	for (int32 Index = 0; Index < ARRAY_COUNT(ComponentTypeData); Index++) {
		UntypedComponentList List = CreateComponentList(
			GetStringId(ComponentTypeData[Index].Name),
			ComponentTypeData[Index].Size,
			ComponentTypeData[Index].Capacity);
		hmputs(NewState->ComponentLists, List);
	}

	NewState->AllTimeHighGeneration = 1;
	NewState->AllTimeHighGenerationFirstIndex = NONE;
}

void DestroyGameWorld(GameWorld* World)
{
	// arrfree(World->Entities);
	arrfree(World->ActiveEntities);
	arrfree(World->EntityGenerations);
	arrfree(World->AvailableIndexStack);
	ZERO_STRUCT(World);
	free(World);
}

EntityId CreateEntity(GameWorld* World)
{
	int32 EntityIndex = NONE;
	if (arrlen(World->AvailableIndexStack) > 0) {
		EntityIndex = arrpop(World->AvailableIndexStack);
	} else {
		EntityIndex = arrlen(World->ActiveEntities);
		// arrput(World->Entities, (GameEntity){0});
		arrput(World->EntityGenerations, World->AllTimeHighGeneration);
	}
	int32 EntityGeneration = World->EntityGenerations[EntityIndex];
	EntityId Result = ENTITY_ID(EntityIndex, EntityGeneration);
	arrput(World->ActiveEntities, Result);
	return Result;
}

void DestroyEntity(GameWorld* World, EntityId Entity)
{
	ASSERT(EntityIdIsValid(World, Entity));

	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	int32 EntityGeneration = ENTITY_ID_GENERATION(Entity);

	arrput(World->AvailableIndexStack, EntityIndex);
	int32 Search = BinarySearch(Entity.RawValue, (int32*)World->ActiveEntities, arrlen(World->ActiveEntities));
	if (Search != NONE) {
		arrdel(World->ActiveEntities, Search);
	} else {
		LogError("GameWorld:Entities:DestroyEntity: Could not find entity '%d' in ActiveEntities", Entity.RawValue);
	}

	int32 NextGeneration = EntityGeneration + 1;
	if (NextGeneration <= World->AllTimeHighGeneration) {
		if (EntityIndex < World->AllTimeHighGenerationFirstIndex) {
			World->AllTimeHighGeneration++;
			NextGeneration = World->AllTimeHighGeneration;
			World->AllTimeHighGenerationFirstIndex = EntityIndex;
		} else {
			NextGeneration = World->AllTimeHighGeneration;
		}
	} else {
		World->AllTimeHighGeneration = NextGeneration;
		World->AllTimeHighGenerationFirstIndex = EntityIndex;
	}

	World->EntityGenerations[EntityIndex] = NextGeneration;
}

bool EntityIdIsValid(GameWorld* World, EntityId Entity)
{
	if (ENTITY_ID_EQ(Entity, ENTITY_ID_INVALID)) {
		return false;
	}

	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	if (!VALID_INDEX(EntityIndex, arrlen(World->ActiveEntities))) {
		return false;
	}

	int32 EntityGeneration = ENTITY_ID_GENERATION(Entity);
	int32 ExpectedGeneration = World->EntityGenerations[EntityIndex];
	return EntityGeneration == ExpectedGeneration;
}

static inline UntypedComponentList* _GetComponentList(GameWorld* World, StringId ComponentId)
{
	UntypedComponentList* List = hmgetp(World->ComponentLists, ComponentId);
	ASSERT(List != NULL);
	return List;
}

void* EntityAddComponent(GameWorld* World, EntityId Entity, StringId ComponentId, const void* ComponentData)
{
	ASSERT(EntityIdIsValid(World, Entity));
	void* Component = NULL;
	UntypedComponentList* List = _GetComponentList(World, ComponentId);
	if (List->ComponentMemory != NULL) {
		Component = ComponentListAdd(List, Entity, ComponentData);
	} else {
		LogError("List not found for ComponentId '%s'.", StringIdCStr(ComponentId));
	}
	return Component;
}

void EntityRemoveComponent(GameWorld* World, EntityId Entity, StringId ComponentId)
{
	ASSERT(EntityIdIsValid(World, Entity));
	UntypedComponentList* List = _GetComponentList(World, ComponentId);
	if (List != NULL) {
		ComponentListRemove(List, Entity);
	} else {
		LogError("List not found for ComponentId '%s'.", StringIdCStr(ComponentId));
	}
}

void* EntityGetComponent(GameWorld* World, EntityId Entity, StringId ComponentId)
{
	ASSERT(EntityIdIsValid(World, Entity));
	void* Component = NULL;
	UntypedComponentList* List = _GetComponentList(World, ComponentId);
	if (List != NULL) {
		Component = ComponentListGet(List, Entity);
	} else {
		LogError("List not found for ComponentId '%s'.", StringIdCStr(ComponentId));
	}
	return Component;
}

bool EntityHasComponent(GameWorld* World, EntityId Entity, StringId ComponentId)
{
	ASSERT(EntityIdIsValid(World, Entity));
	bool Result = false;
	UntypedComponentList* List = _GetComponentList(World, ComponentId);
	if (List != NULL) {
		Result = ComponentListHas(List, Entity);
	} else {
		LogError("List not found for ComponentId '%s'.", StringIdCStr(ComponentId));
	}
	return Result;
}

#define ADD_COMPONENT_IMPLEMENTATION(Type)                                                                             \
	ADD_COMPONENT_PROTOTYPE(Type)                                                                                      \
	{                                                                                                                  \
		return (COMPONENT_NAME(Type)*)EntityAddComponent(World, Entity, CID.Type, ComponentData);                      \
	}

#define REMOVE_COMPONENT_IMPLEMENTATION(Type)                                                                          \
	REMOVE_COMPONENT_PROTOTYPE(Type)                                                                                   \
	{                                                                                                                  \
		EntityRemoveComponent(World, Entity, CID.Type);                                                                \
	}

#define GET_COMPONENT_IMPLEMENTATION(Type)                                                                             \
	GET_COMPONENT_PROTOTYPE(Type)                                                                                      \
	{                                                                                                                  \
		return (COMPONENT_NAME(Type)*)EntityGetComponent(World, Entity, CID.Type);                                     \
	}

#define HAS_COMPONENT_IMPLEMENTATION(Type)                                                                             \
	HAS_COMPONENT_PROTOTYPE(Type)                                                                                      \
	{                                                                                                                  \
		return EntityHasComponent(World, Entity, CID.Type);                                                            \
	}

#define COMPONENT_INTERFACE_IMPLEMENTATION(Type)                                                                       \
	ADD_COMPONENT_IMPLEMENTATION(Type)                                                                                 \
	REMOVE_COMPONENT_IMPLEMENTATION(Type) GET_COMPONENT_IMPLEMENTATION(Type) HAS_COMPONENT_IMPLEMENTATION(Type)

COMPONENT_INTERFACE_IMPLEMENTATION(Transform)
COMPONENT_INTERFACE_IMPLEMENTATION(Sprite)
COMPONENT_INTERFACE_IMPLEMENTATION(Collider)
