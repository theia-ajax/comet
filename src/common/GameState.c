#include "GameState.h"

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

static bool GStaticGameStateDataInitialized = false;
ComponentIdSet ComponentIds = {0};

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

void* ComponentListAdd(UntypedComponentList* Self, EntityId Entity, const void* Component)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	int32 NewIndex = Self->Count;

	ASSERT(EntityIndex <= arrlen(Self->Indices));
	ASSERT(NewIndex <= arrlen(Self->Entities));
	ASSERT(Self->Indices[EntityIndex] == 0);

	if (EntityIndex == arrlen(Self->Indices)) {
		arrput(Self->Indices, NewIndex);
	} else {
		Self->Indices[EntityIndex] = NewIndex;
	}

	if (NewIndex == arrlen(Self->Entities)) {
		arrput(Self->Entities, Entity);
	} else {
		Self->Entities[NewIndex] = Entity;
	}

	void* Storage = ((uint8*)Self->ComponentMemory) + NewIndex * Self->ComponentSize;
	if (Component != NULL) {
		memcpy(Storage, Component, Self->ComponentSize);
	} else {
		memset(Storage, 0, Self->ComponentSize);
	}
	Self->Count++;
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

const int32 KInitialEntityCapacity = 32;
const struct {
	const char* Name;
	int32 Size;
	int32 Capacity;
} ComponentTypeData[] = {
	{"Position", sizeof(Vec2), KInitialEntityCapacity},
	{"Rotation", sizeof(flt32), KInitialEntityCapacity},
};

typedef struct GameState {
	// GameEntity* Entities;
	EntityId* ActiveEntities;
	int32* EntityGenerations;
	int32* AvailableIndexStack;

	UntypedComponentList* ComponentLists;

	// New band/album name
	int32 AllTimeHighGeneration;
	int32 AllTimeHighGenerationFirstIndex;
} GameState;

GameState* CreateGameState(void)
{
	if (!GStaticGameStateDataInitialized) {
		GStaticGameStateDataInitialized = true;
		ComponentIds.Transform = GetStringId("Transform");
	}

	GameState* NewState = (GameState*)malloc(sizeof(GameState));
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

void DestroyGameState(GameState* Self)
{
	// arrfree(Self->Entities);
	arrfree(Self->ActiveEntities);
	arrfree(Self->EntityGenerations);
	arrfree(Self->AvailableIndexStack);
	ZERO_STRUCT(Self);
	free(Self);
}

EntityId GameStateCreateEntity(GameState* Self)
{
	int32 EntityIndex = NONE;
	if (arrlen(Self->AvailableIndexStack) > 0) {
		EntityIndex = arrpop(Self->AvailableIndexStack);
	} else {
		EntityIndex = arrlen(Self->ActiveEntities);
		// arrput(Self->Entities, (GameEntity){0});
		arrput(Self->EntityGenerations, Self->AllTimeHighGeneration);
	}
	int32 EntityGeneration = Self->EntityGenerations[EntityIndex];
	EntityId Result = ENTITY_ID(EntityIndex, EntityGeneration);
	arrput(Self->ActiveEntities, Result);
	return Result;
}

void GameStateDestroyEntity(GameState* Self, EntityId Entity)
{
	ASSERT(GameStateEntityIdIsValid(Self, Entity));

	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	int32 EntityGeneration = ENTITY_ID_GENERATION(Entity);

	arrput(Self->AvailableIndexStack, EntityIndex);
	int32 Search = BinarySearch(Entity.RawValue, (int32*)Self->ActiveEntities, arrlen(Self->ActiveEntities));
	if (Search != NONE) {
		arrdel(Self->ActiveEntities, Search);
	} else {
		LogError("GameState:Entities:DestroyEntity: Could not find entity '%d' in ActiveEntities", Entity.RawValue);
	}

	int32 NextGeneration = EntityGeneration + 1;
	if (NextGeneration <= Self->AllTimeHighGeneration) {
		if (EntityIndex < Self->AllTimeHighGenerationFirstIndex) {
			Self->AllTimeHighGeneration++;
			NextGeneration = Self->AllTimeHighGeneration;
			Self->AllTimeHighGenerationFirstIndex = EntityIndex;
		} else {
			NextGeneration = Self->AllTimeHighGeneration;
		}
	} else {
		Self->AllTimeHighGeneration = NextGeneration;
		Self->AllTimeHighGenerationFirstIndex = EntityIndex;
	}

	Self->EntityGenerations[EntityIndex] = NextGeneration;
}

bool GameStateEntityIdIsValid(GameState* Self, EntityId Entity)
{
	if (ENTITY_ID_EQ(Entity, ENTITY_ID_INVALID)) {
		return false;
	}

	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	if (!VALID_INDEX(EntityIndex, arrlen(Self->ActiveEntities))) {
		return false;
	}

	int32 EntityGeneration = ENTITY_ID_GENERATION(Entity);
	int32 ExpectedGeneration = Self->EntityGenerations[EntityIndex];
	return EntityGeneration == ExpectedGeneration;
}

static inline UntypedComponentList* _GetComponentList(GameState* Self, StringId ComponentId)
{
	UntypedComponentList* List = hmgetp(Self->ComponentLists, ComponentId);
	ASSERT(List != NULL);
	return List;
}

void* EntityAddComponent(GameState* State, EntityId Entity, StringId ComponentId, const void* ComponentData)
{
	ASSERT(GameStateEntityIdIsValid(State, Entity));
	void* Component = NULL;
	UntypedComponentList* List = _GetComponentList(State, ComponentId);
	if (List != NULL) {
		Component = ComponentListAdd(List, Entity, ComponentData);
	} else {
		LogError("List not found for ComponentId '%s'.", StringIdCStr(ComponentId));
	}
	return Component;
}

void EntityRemoveComponent(GameState* State, EntityId Entity, StringId ComponentId)
{
	ASSERT(GameStateEntityIdIsValid(State, Entity));
	UntypedComponentList* List = _GetComponentList(State, ComponentId);
	if (List != NULL) {
		ComponentListRemove(List, Entity);
	} else {
		LogError("List not found for ComponentId '%s'.", StringIdCStr(ComponentId));
	}
}

void* EntityGetComponent(GameState* State, EntityId Entity, StringId ComponentId)
{
	ASSERT(GameStateEntityIdIsValid(State, Entity));
	void* Component = NULL;
	UntypedComponentList* List = _GetComponentList(State, ComponentId);
	if (List != NULL) {
		Component = ComponentListGet(List, Entity);
	} else {
		LogError("List not found for ComponentId '%s'.", StringIdCStr(ComponentId));
	}
	return Component;
}

bool EntityHasComponent(GameState* State, EntityId Entity, StringId ComponentId)
{
	ASSERT(GameStateEntityIdIsValid(State, Entity));
	bool Result = false;
	UntypedComponentList* List = _GetComponentList(State, ComponentId);
	if (List != NULL) {
		Result = ComponentListHas(List, Entity);
	} else {
		LogError("List not found for ComponentId '%s'.", StringIdCStr(ComponentId));
	}
	return Result;
}
