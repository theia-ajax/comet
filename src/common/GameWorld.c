#include "GameWorld.h"

#include <stb_ds.h>

#include "Log.h"
#include "Util.h"

// Constants
// -------------------------------------------------------
const int32 KInitialEntityCapacity = 32;
const int32 KDefaultInitialComponentCapacity = 32;

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

#define COMPONENT_NAME_ENTRY(Type) #Type,
static const char* ComponentTypeNames[] = {FOR_EACH(COMPONENT_NAME_ENTRY, COMPONENT_TYPE_LIST)};
_Static_assert(ARRAY_COUNT(ComponentTypeNames) == ComponentType_Count, "");

// TODO: Revisit this
const int32 ComponentInitialCapacities[ComponentType_Count] = {
	0, // Transform
};

#define COMPONENT_TYPE_DATA_ENTRY(Type) {#Type, sizeof(CAT(Type, Component))},
const struct {
	const char* Name;
	int32 Size;
} ComponentTypeData[] = {FOR_EACH(COMPONENT_TYPE_DATA_ENTRY, COMPONENT_TYPE_LIST)};

_Static_assert(
	ARRAY_COUNT(ComponentTypeData) == ComponentType_Count,
	"ComponentIdSet missing component ID or ComponentTypeData missing type data");

// Private Definitions
// -------------------------------------------------------
typedef struct UntypedComponentList {
	ComponentType Type;
	int32 Count;
	int32 Capacity;
	int32 ComponentSize;
	void* ComponentMemory;
	int32* Indices;
	EntityId* Entities;
} UntypedComponentList;
#define ComponentListCast(List, Type) ((Type)*)((List).ComponentMemory)

typedef struct GameWorld {
	// GameEntity* Entities;
	EntityId* ActiveEntities;
	EntitySignature* EntitySignatures;
	int32* EntityGenerations;
	int32* AvailableIndexStack;

	UntypedComponentList ComponentLists[ComponentType_Count];

	// New band/album name
	int32 AllTimeHighGeneration;
	int32 AllTimeHighGenerationFirstIndex;
} GameWorld;

// Component list
static UntypedComponentList CreateComponentList(ComponentType Type, int32 ComponentSize, int32 Capacity);
static inline void* ComponentMemory(UntypedComponentList* Self, int32 Index);
static void DestroyComponentList(UntypedComponentList* Self);
static void* ComponentListAdd(UntypedComponentList* List, EntityId Entity, const void* Component);
static void ComponentListRemove(UntypedComponentList* Self, EntityId Entity);
static void* ComponentListGet(UntypedComponentList* Self, EntityId Entity);
static bool ComponentListHas(UntypedComponentList* Self, EntityId Entity);

// Entity-component internal API, all higher level entity-component macros call into here (AddComponent,
// RemoveComponent, etc...)
static void* EntityAddComponent(GameWorld* World, EntityId Entity, ComponentType Type, const void* ComponentData);
static void EntityRemoveComponent(GameWorld* World, EntityId Entity, ComponentType Type);
static void* EntityGetComponent(GameWorld* World, EntityId Entity, ComponentType Type);
static void* EntityTryGetComponent(GameWorld* World, EntityId Entity, ComponentType Type);
static bool EntityHasComponent(GameWorld* World, EntityId Entity, ComponentType Type);

// Public Implementations
// -------------------------------------------------------
GameWorld* CreateGameWorld(void)
{
	GameWorld* NewState = (GameWorld*)malloc(sizeof(GameWorld));
	ZERO_STRUCT(NewState);

	// arrsetcap(NewState->Entities, KInitialEntityCapacity);
	arrsetcap(NewState->ActiveEntities, KInitialEntityCapacity);
	arrsetcap(NewState->EntitySignatures, KInitialEntityCapacity);
	arrsetcap(NewState->EntityGenerations, KInitialEntityCapacity);
	arrsetcap(NewState->AvailableIndexStack, 32);

	for (int32 Index = 0; Index < ARRAY_COUNT(ComponentTypeData); Index++) {
		int32 Capacity = ComponentInitialCapacities[Index];
		if (Capacity == 0) {
			Capacity = KDefaultInitialComponentCapacity;
			LogWarning(
				"Created %s ComponentList with default initial capacity of %d", ComponentTypeName(Index), Capacity);
		} else {
			LogWarning("Created %s ComponentList with initial capacity of %d", ComponentTypeName(Index), Capacity);
		}
		Capacity = (Capacity != 0) ? Capacity : KDefaultInitialComponentCapacity;
		NewState->ComponentLists[Index] = CreateComponentList(Index, ComponentTypeData[Index].Size, Capacity);
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
		arrput(World->EntitySignatures, (EntitySignature){0});
	}
	int32 EntityGeneration = World->EntityGenerations[EntityIndex];
	EntityId Result = ENTITY_ID(EntityIndex, EntityGeneration);
	arrput(World->ActiveEntities, Result);
	return Result;
}

void DestroyEntity(GameWorld* World, EntityId Entity)
{
	ASSERT(EntityIdIsValid(World, Entity));

	const int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	const int32 EntityGeneration = ENTITY_ID_GENERATION(Entity);

	arrput(World->AvailableIndexStack, EntityIndex);
	const int32 Search = BinarySearch(Entity.RawValue, (int32*)World->ActiveEntities, arrlen(World->ActiveEntities));
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

	World->EntitySignatures[EntityIndex] = (EntitySignature){0};
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

EntitySignature EntityGetSignature(GameWorld* World, EntityId Entity)
{
	ASSERT(EntityIdIsValid(World, Entity));
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	return World->EntitySignatures[EntityIndex];
}

bool EntitySignaturePassesFilter(EntitySignature Signature, EntitySignature Required, EntitySignature Rejected)
{
	bool HasAllRequired = (Signature.RawValue & Required.RawValue) == Required.RawValue;
	bool HasAnyRejected = (Signature.RawValue & Rejected.RawValue) != 0;
	return HasAllRequired && !HasAnyRejected;
}

EntityId* WorldEntitiesBegin(GameWorld* World)
{
	return World->ActiveEntities;
}

EntityId* WorldEntitiesEnd(GameWorld* World)
{
	return arrend(World->ActiveEntities);
}

// TODO: This is implemented in a weird place, maybe move this to a component specific translation unit
inline const char* ComponentTypeName(ComponentType Type)
{
	return ComponentTypeNames[Type];
}

// Private Implementations
// -------------------------------------------------------
static UntypedComponentList CreateComponentList(ComponentType Type, int32 ComponentSize, int32 Capacity)
{
	UntypedComponentList Self;
	ZERO_STRUCT(&Self);
	Self.Type = Type;
	Self.ComponentSize = ComponentSize;
	Self.Capacity = Capacity;
	Self.ComponentMemory = malloc(Self.ComponentSize * Self.Capacity);
	ASSERT(Self.ComponentMemory);
	arrsetcap(Self.Indices, Self.Capacity);
	arrsetcap(Self.Entities, Self.Capacity);
	memset(Self.Indices, NONE, sizeof(*Self.Indices) * arrcap(Self.Indices));
	memset(Self.Entities, 0, sizeof(*Self.Entities) * arrcap(Self.Entities));
	return Self;
}

static inline void* ComponentMemory(UntypedComponentList* Self, int32 Index)
{
	return ((uint8*)Self->ComponentMemory) + (Index * Self->ComponentSize);
}

static void DestroyComponentList(UntypedComponentList* Self)
{
	free(Self->ComponentMemory);
	arrfree(Self->Indices);
	arrfree(Self->Entities);
	ZERO_STRUCT(Self);
}

static void* ComponentListAdd(UntypedComponentList* List, EntityId Entity, const void* Component)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	int32 NewIndex = List->Count;

	ASSERT(EntityIndex <= arrlen(List->Indices) && "Entity has invalid index.");
	ASSERT(NewIndex <= arrlen(List->Entities));
	ASSERT(List->Indices[EntityIndex] == NONE && "Entity already has component");

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

static void ComponentListRemove(UntypedComponentList* Self, EntityId Entity)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	ASSERT(Self->Indices[EntityIndex] != NONE);

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

static void* ComponentListGet(UntypedComponentList* Self, EntityId Entity)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	ASSERT(ENTITY_ID_NEQ(Self->Entities[EntityIndex], ENTITY_ID_INVALID));

	int32 ComponentIndex = Self->Indices[EntityIndex];
	void* Result = ComponentMemory(Self, ComponentIndex);
	return Result;
}

static bool ComponentListHas(UntypedComponentList* Self, EntityId Entity)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	return Self->Indices[EntityIndex] != NONE;
}

static inline UntypedComponentList* _GetComponentList(GameWorld* World, ComponentType Type)
{
	ASSERT(VALID_INDEX(Type, ComponentType_Count));
	return &World->ComponentLists[Type];
}

static void* EntityAddComponent(GameWorld* World, EntityId Entity, ComponentType Type, const void* ComponentData)
{
	ASSERT(EntityIdIsValid(World, Entity));
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	World->EntitySignatures[EntityIndex].RawValue |= BIT_FLAG64(Type);
	return ComponentListAdd(_GetComponentList(World, Type), Entity, ComponentData);
}

static void EntityRemoveComponent(GameWorld* World, EntityId Entity, ComponentType Type)
{
	ASSERT(EntityIdIsValid(World, Entity));
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	World->EntitySignatures[EntityIndex].RawValue &= ~(BIT_FLAG64(Type));
	ComponentListRemove(_GetComponentList(World, Type), Entity);
}

static void* EntityGetComponent(GameWorld* World, EntityId Entity, ComponentType Type)
{
	ASSERT(EntityIdIsValid(World, Entity));
	return ComponentListGet(_GetComponentList(World, Type), Entity);
}

static void* EntityTryGetComponent(GameWorld* World, EntityId Entity, ComponentType Type)
{
	ASSERT(EntityIdIsValid(World, Entity));
	void* Result = NULL;
	if (EntityHasComponent(World, Entity, Type)) {
		Result = ComponentListGet(_GetComponentList(World, Type), Entity);
	}
	return Result;
}

static bool EntityHasComponent(GameWorld* World, EntityId Entity, ComponentType Type)
{
	ASSERT(EntityIdIsValid(World, Entity));
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	return (World->EntitySignatures[EntityIndex].RawValue & BIT_FLAG64(Type)) != 0;
}

#define ADD_COMPONENT_IMPLEMENTATION(Type)                                                                             \
	ADD_COMPONENT_PROTOTYPE(Type)                                                                                      \
	{                                                                                                                  \
		return (COMPONENT_NAME(Type)*)EntityAddComponent(World, Entity, CAT(ComponentType_, Type), ComponentData);     \
	}

#define REMOVE_COMPONENT_IMPLEMENTATION(Type)                                                                          \
	REMOVE_COMPONENT_PROTOTYPE(Type)                                                                                   \
	{                                                                                                                  \
		EntityRemoveComponent(World, Entity, CAT(ComponentType_, Type));                                               \
	}

#define GET_COMPONENT_IMPLEMENTATION(Type)                                                                             \
	GET_COMPONENT_PROTOTYPE(Type)                                                                                      \
	{                                                                                                                  \
		return (COMPONENT_NAME(Type)*)EntityGetComponent(World, Entity, CAT(ComponentType_, Type));                    \
	}

#define TRYGET_COMPONENT_IMPLEMENTATION(Type)                                                                          \
	TRYGET_COMPONENT_PROTOTYPE(Type)                                                                                   \
	{                                                                                                                  \
		return (COMPONENT_NAME(Type)*)EntityTryGetComponent(World, Entity, COMPONENT_TYPE_ENUM_VALUE(Type));           \
	}

#define HAS_COMPONENT_IMPLEMENTATION(Type)                                                                             \
	HAS_COMPONENT_PROTOTYPE(Type)                                                                                      \
	{                                                                                                                  \
		return EntityHasComponent(World, Entity, CAT(ComponentType_, Type));                                           \
	}

#define COMPONENT_INTERFACE_IMPLEMENTATION(Type)                                                                       \
	ADD_COMPONENT_IMPLEMENTATION(Type);                                                                                \
	REMOVE_COMPONENT_IMPLEMENTATION(Type);                                                                             \
	GET_COMPONENT_IMPLEMENTATION(Type);                                                                                \
	TRYGET_COMPONENT_IMPLEMENTATION(Type);                                                                             \
	HAS_COMPONENT_IMPLEMENTATION(Type);

// For every component type defined in ComponentTypes.h will create corresponding type-safe entity-component interface
// implementations e.g.:
// EntityAddTransformComponent, EntityRemoveTransformComponent, etc...
// _Generic entity-component interface defined in GameWorld.h calls these generated functions
FOR_EACH(COMPONENT_INTERFACE_IMPLEMENTATION, COMPONENT_TYPE_LIST);
