#include "GameWorld.h"

#include <stb_ds.h>

#include "Log.h"
#include "Util.h"

// Constants
// -------------------------------------------------------
const int32 KInitialEntityCapacity = 4;
const int32 KDefaultInitialComponentCapacity = 4;

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
static UntypedComponentList CreateComponentList(
	ComponentType Type,
	int32 ComponentSize,
	int32 Capacity,
	int32 EntityCapacity);
static inline void* ComponentMemory(UntypedComponentList* List, int32 Index);
static void DestroyComponentList(UntypedComponentList* List);
static void* ComponentListAdd(UntypedComponentList* List, EntityId Entity);
static void ComponentListRemove(UntypedComponentList* List, EntityId Entity);
static void* ComponentListGet(UntypedComponentList* List, EntityId Entity);
static bool ComponentListHas(UntypedComponentList* List, EntityId Entity);

// Entity-component internal API, all higher level entity-component macros call into here (AddComponent,
// RemoveComponent, etc...)
static void* EntityAddComponent(GameWorld* World, EntityId Entity, ComponentType Type);
static void EntityRemoveComponent(GameWorld* World, EntityId Entity, ComponentType Type);
static void* EntityGetComponent(GameWorld* World, EntityId Entity, ComponentType Type);
static void* EntityTryGetComponent(GameWorld* World, EntityId Entity, ComponentType Type);
static bool EntityHasComponent(GameWorld* World, EntityId Entity, ComponentType Type);

// Public Implementations
// -------------------------------------------------------
GameWorld* CreateGameWorld(void)
{
	GameWorld* NewState = (GameWorld*)malloc(sizeof(GameWorld));

	if (NewState == NULL) {
		LogError("Unable to allocate game world");
		exit(1);
	}

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
		NewState->ComponentLists[Index] =
			CreateComponentList(Index, ComponentTypeData[Index].Size, Capacity, KInitialEntityCapacity);
	}

	NewState->AllTimeHighGeneration = 1;
	NewState->AllTimeHighGenerationFirstIndex = NONE;

	return NewState;
}

void DestroyGameWorld(GameWorld* World)
{
	// arrfree(World->Entities);
	arrfree(World->ActiveEntities);
	arrfree(World->EntityGenerations);
	arrfree(World->EntitySignatures);
	arrfree(World->AvailableIndexStack);
	ZERO_STRUCT(World);
	free(World);
}

EntityId CreateEntity(GameWorld* World)
{
	int32 EntityIndex = NONE;
	int Z = arrlen(World->AvailableIndexStack);
	if (arrlen(World->AvailableIndexStack) > 0) {
		EntityIndex = arrpop(World->AvailableIndexStack);
	} else {
		EntityIndex = arrlen(World->ActiveEntities);
		arrput(World->EntityGenerations, World->AllTimeHighGeneration);
		arrput(World->EntitySignatures, (EntitySignature){0});
	}
	int32 EntityGeneration = World->EntityGenerations[EntityIndex];
	EntityId Result = ENTITY_ID(EntityIndex, EntityGeneration);
	int32 LastCapacity = arrcap(World->ActiveEntities);
	arrput(World->ActiveEntities, Result);
	if (arrcap(World->ActiveEntities) > LastCapacity) {
		int32 NewLength = arrcap(World->ActiveEntities);
		for (int32 ListIndex = 0; ListIndex < ARRAY_COUNT(World->ComponentLists); ListIndex++) {
			int32 OldLength = arrlen(World->ComponentLists[ListIndex].Indices);
			arrsetlen(World->ComponentLists[ListIndex].Indices, NewLength);
			memset(
				&World->ComponentLists[ListIndex].Indices[OldLength],
				NONE,
				(NewLength - OldLength) * sizeof(*World->ComponentLists[ListIndex].Indices));
		}
		LogWarning(
			"GameWorld:CreateEntity: Expanding ActiveEntities from %d to %d",
			LastCapacity,
			arrcap(World->ActiveEntities));
	}
	LogInfo("Create Entity %d[%u:%u]", Result.RawValue, ENTITY_ID_INDEX(Result), ENTITY_ID_GENERATION(Result));
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

		for (int32 ListIndex = 0; ListIndex < ARRAY_COUNT(World->ComponentLists); ListIndex++) {
			if (ComponentListHas(&World->ComponentLists[ListIndex], Entity)) {
				ComponentListRemove(&World->ComponentLists[ListIndex], Entity);
			}
		}
	} else {
		LogError("GameWorld:Entities:DestroyEntity: Could not find entity '%d' in ActiveEntities", Entity.RawValue);
	}
}

bool EntityIdIsValid(GameWorld* World, EntityId Entity)
{
	ASSERT(!ENTITY_ID_EQ(Entity, ENTITY_ID_INVALID));
	if (ENTITY_ID_EQ(Entity, ENTITY_ID_INVALID)) {
		return false;
	}

	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	ASSERT(VALID_INDEX(EntityIndex, arrlen(World->EntityGenerations)));
	if (!VALID_INDEX(EntityIndex, arrlen(World->EntityGenerations))) {
		return false;
	}

	int32 EntityGeneration = ENTITY_ID_GENERATION(Entity);
	int32 ExpectedGeneration = World->EntityGenerations[EntityIndex];
	ASSERT(EntityGeneration == ExpectedGeneration);
	return EntityGeneration == ExpectedGeneration;
}

EntitySignature EntityGetSignature(GameWorld* World, EntityId Entity)
{
	ASSERT(EntityIdIsValid(World, Entity));
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	return World->EntitySignatures[EntityIndex];
}

inline bool EntitySignaturePassesFilter(EntitySignature Signature, EntitySignature Required, EntitySignature Rejected)
{
	return (Signature.RawValue & Required.RawValue & Rejected.RawValue) == Required.RawValue;
}

EntityId* WorldEntitiesBegin(GameWorld* World)
{
	return World->ActiveEntities;
}

EntityId* WorldEntitiesEnd(GameWorld* World)
{
	return arrend(World->ActiveEntities);
}

EntityId* WorldQueryEntities(GameWorld* World, EntitySignature Required, EntitySignature Rejected)
{
	EntityId* Entities = NULL;
	arrsetcap(Entities, arrlen(World->ActiveEntities));

	for (int32 Index = 0; Index < arrlen(World->EntitySignatures); Index++) {
		EntitySignature Signature = World->EntitySignatures[Index];
		if (EntitySignaturePassesFilter(Signature, Required, Rejected)) {
			arrput(Entities, World->ActiveEntities[Index]);
		}
	}

	return Entities;
}

void WorldQueryFree(EntityId* Query)
{
	arrfree(Query);
}

int32 WorldEntityCount(GameWorld* World)
{
	return (int32)arrlen(World->ActiveEntities);
}

// TODO: This is implemented in a weird place, maybe move this to a component specific translation unit
inline const char* ComponentTypeName(ComponentType Type)
{
	return ComponentTypeNames[Type];
}

// Private Implementations
// -------------------------------------------------------
static UntypedComponentList CreateComponentList(
	ComponentType Type,
	int32 ComponentSize,
	int32 Capacity,
	int32 EntityCapacity)
{
	UntypedComponentList List;
	ZERO_STRUCT(&List);
	List.Type = Type;
	List.ComponentSize = ComponentSize;
	List.Capacity = Capacity;
	List.ComponentMemory = malloc(List.ComponentSize * List.Capacity);
	memset(List.ComponentMemory, 0, List.ComponentSize * List.Capacity);
	ASSERT(List.ComponentMemory);
	List.Entities = malloc(List.Capacity * sizeof(*List.Entities));
	ASSERT(List.Entities);
	arrsetlen(List.Indices, EntityCapacity);
	memset(List.Indices, NONE, sizeof(*List.Indices) * arrcap(List.Indices));
	memset(List.Entities, 0, sizeof(*List.Entities) * List.Capacity);
	return List;
}

static inline void* ComponentMemory(UntypedComponentList* List, int32 Index)
{
	return ((uint8*)List->ComponentMemory) + (Index * List->ComponentSize);
}

static void DestroyComponentList(UntypedComponentList* List)
{
	free(List->ComponentMemory);
	free(List->Entities);
	arrfree(List->Indices);
	ZERO_STRUCT(List);
}

static void* ComponentListAdd(UntypedComponentList* List, EntityId Entity)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	int32 NewIndex = List->Count;

	ptrdiff_t IndexCount = arrlen(List->Indices);
	ASSERT(EntityIndex <= arrlen(List->Indices) && "Entity has invalid index.");

	ASSERT(List->Indices[EntityIndex] == NONE && "Entity already has component");
	List->Indices[EntityIndex] = NewIndex;

	if (NewIndex == List->Capacity) {
		int32 LastCapacity = List->Capacity;
		List->Capacity *= 2;
		List->ComponentMemory = realloc(List->ComponentMemory, List->Capacity * List->ComponentSize);
		memset(
			(uint8*)List->ComponentMemory + LastCapacity * List->ComponentSize,
			0,
			(List->Capacity - LastCapacity) * List->ComponentSize);
		List->Entities = realloc(List->Entities, List->Capacity * sizeof(*List->Entities));
		memset(List->Entities + LastCapacity, 0, (List->Capacity - LastCapacity) * sizeof(*List->Entities));
		LogWarning(
			"GameWorld:ComponentListAdd<%s>: Capacity reached, increasing from %d to %d",
			ComponentTypeName(List->Type),
			LastCapacity,
			List->Capacity);
	}

	ASSERT(List->Entities[NewIndex].RawValue == 0);
	List->Entities[NewIndex] = Entity;

	void* Storage = ((uint8*)List->ComponentMemory) + NewIndex * List->ComponentSize;
	List->Count++;
	return Storage;
}

static void ComponentListRemove(UntypedComponentList* List, EntityId Entity)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	ASSERT(List->Indices[EntityIndex] != NONE);

	int32 RemovedIndex = List->Indices[EntityIndex];
	int32 LastIndex = List->Count - 1;

	void* RemovedMemory = ComponentMemory(List, RemovedIndex);
	void* LastMemory = ComponentMemory(List, List->Count - 1);
	memcpy(RemovedMemory, LastMemory, List->ComponentSize);
	memset(LastMemory, 0, List->ComponentSize);
	List->Count--;

	EntityId LastEntity = List->Entities[LastIndex];
	int32 LastEntityIndex = ENTITY_ID_INDEX(LastEntity);
	List->Indices[LastEntityIndex] = RemovedIndex;
	List->Entities[RemovedIndex] = LastEntity;

	List->Indices[EntityIndex] = NONE;
	List->Entities[LastIndex] = ENTITY_ID_INVALID;
}

static void* ComponentListGet(UntypedComponentList* List, EntityId Entity)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	int32 ComponentIndex = List->Indices[EntityIndex];
	ASSERT(ENTITY_ID_NEQ(List->Entities[ComponentIndex], ENTITY_ID_INVALID));

	void* Result = ComponentMemory(List, ComponentIndex);
	return Result;
}

static bool ComponentListHas(UntypedComponentList* List, EntityId Entity)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	return List->Indices[EntityIndex] != NONE;
}

static inline UntypedComponentList* _GetComponentList(GameWorld* World, ComponentType Type)
{
	ASSERT(VALID_INDEX(Type, ComponentType_Count));
	return &World->ComponentLists[Type];
}

static void* EntityAddComponent(GameWorld* World, EntityId Entity, ComponentType Type)
{
	ASSERT(EntityIdIsValid(World, Entity));
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	World->EntitySignatures[EntityIndex].RawValue |= BIT_FLAG64(Type);
	LogInfo("GameWorld:EntityAddComponent: added %s component to entity %d", ComponentTypeName(Type), Entity.RawValue);
	return ComponentListAdd(_GetComponentList(World, Type), Entity);
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
		return (COMPONENT_NAME(Type)*)EntityAddComponent(World, Entity, CAT(ComponentType_, Type));                    \
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
