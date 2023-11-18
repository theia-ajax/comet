#include "GameWorld.h"

#include <stb_ds.h>

#include "Log.h"
#include "Util.h"

// Constants
// -------------------------------------------------------
const int32 KInitialEntityCapacity = 256;
const int32 KDefaultInitialComponentCapacity = 32;

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
	GameWorld* World;
	ComponentType Type;
	int32 Count;
	int32 Capacity;
	int32 ComponentSize;
	void* ComponentMemory;
	int32* Indices;
	EntityId* Entities;
} UntypedComponentList;
#define ComponentListCast(List, Type) ((Type)*)((List).ComponentMemory)

typedef enum GameWorldCommandOp {
	GameWorldCommandOp_CreateEntity,
	GameWorldCommandOp_DestroyEntity,
	GameWorldCommandOp_AddComponent,
	GameWorldCommandOp_RemoveComponent,
	GameWorldCommandOp_Count,
} GameWorldCommandOp;

typedef struct GameWorldCommand {
	GameWorldCommandOp Operation;
	EntityId Entity;
	FutureEntityId FutureEntity;
	ComponentType ComponentType;
	void* ComponentStorage;
} GameWorldCommand;

typedef struct GameWorld {
	// GameEntity* Entities;
	EntityId* ActiveEntities;
	EntitySignature* EntitySignatures;
	int32* EntityGenerations;
	int32* AvailableIndexStack;
	int32 EntityCapacity;

	UntypedComponentList ComponentLists[ComponentType_Count];

	bool IsLocked;

	// New band/album name
	int32 AllTimeHighGeneration;
	int32 AllTimeHighGenerationFirstIndex;
} GameWorld;

static void* ReallocComponentMemory(void* Memory, int32 ElementSize, int32 LastCapacity, int32 NewCapacity);
static EntityId* ReallocEntities(EntityId* Entities, int32 LastCapacity, int32 NewCapacity);
static int32* ReallocIndices(int32* Indices, int32 LastCapacity, int32 NewCapacity);

// Component list
static UntypedComponentList CreateComponentList(
	GameWorld* World,
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
static void* EntityQueueAddComponent(GameWorldCommandQueue* Queue, EntityId Entity, ComponentType Type);
static void EntityQueueRemoveComponent(GameWorldCommandQueue* Queue, EntityId Entity, ComponentType Type);
static void* EntityGetComponent(GameWorld* World, EntityId Entity, ComponentType Type);
static void* EntityTryGetComponent(GameWorld* World, EntityId Entity, ComponentType Type);
static bool EntityHasComponent(GameWorld* World, EntityId Entity, ComponentType Type);
static inline UntypedComponentList* _GetComponentList(GameWorld* World, ComponentType Type);

// Public Implementations
// -------------------------------------------------------
GameWorld* CreateGameWorld(void)
{
	GameWorld* NewWorld = (GameWorld*)malloc(sizeof(GameWorld));

	if (NewWorld == NULL) {
		LogError("Unable to allocate game world");
		exit(1);
	}

	ZERO_STRUCT(NewWorld);

	// arrsetcap(NewWorld->Entities, KInitialEntityCapacity);
	arrsetcap(NewWorld->ActiveEntities, KInitialEntityCapacity);
	arrsetcap(NewWorld->EntitySignatures, KInitialEntityCapacity);
	arrsetcap(NewWorld->EntityGenerations, KInitialEntityCapacity);
	arrsetcap(NewWorld->AvailableIndexStack, 32);

	NewWorld->EntityCapacity = arrcap(NewWorld->ActiveEntities);

	for (int32 ComponentIndex = 0; ComponentIndex < ARRAY_COUNT(ComponentTypeData); ComponentIndex++) {
		int32 Capacity = ComponentInitialCapacities[ComponentIndex];
		Capacity = (Capacity != 0) ? Capacity : KDefaultInitialComponentCapacity;
		LogInfo("Created %s ComponentList with initial capacity of %d", ComponentTypeName(ComponentIndex), Capacity);
		NewWorld->ComponentLists[ComponentIndex] = CreateComponentList(
			NewWorld,
			ComponentIndex,
			ComponentTypeData[ComponentIndex].Size,
			Capacity,
			KInitialEntityCapacity);
	}

	NewWorld->AllTimeHighGeneration = 1;
	NewWorld->AllTimeHighGenerationFirstIndex = NONE;

	return NewWorld;
}

void DestroyGameWorld(GameWorld* World)
{
	// arrfree(World->Entities);
	arrfree(World->ActiveEntities);
	arrfree(World->EntityGenerations);
	arrfree(World->EntitySignatures);
	arrfree(World->AvailableIndexStack);

	for (int32 ComponentIndex = 0; ComponentIndex < ARRAY_COUNT(ComponentTypeData); ComponentIndex++) {
		DestroyComponentList(&World->ComponentLists[ComponentIndex]);
	}

	ZERO_STRUCT(World);
	free(World);
}

EntityId CreateEntity(GameWorld* World)
{
	ASSERT(World);
	ASSERT(!World->IsLocked && "Cannot create entities while world is locked.");

	int32 EntityIndex = NONE;
	if (arrlen(World->AvailableIndexStack) > 0) {
		EntityIndex = arrpop(World->AvailableIndexStack);
	} else {
		EntityIndex = arrlen(World->ActiveEntities);
		arrput(World->EntityGenerations, World->AllTimeHighGeneration);
		arrput(World->EntitySignatures, (EntitySignature){0});
	}
	int32 EntityGeneration = World->EntityGenerations[EntityIndex];
	EntityId Result = ENTITY_ID(EntityIndex, EntityGeneration);
	int32 ActiveEntityIndex =
		BinarySearchInsertIndex(Result.RawValue, (int32*)World->ActiveEntities, arrlen(World->ActiveEntities));
	arrins(World->ActiveEntities, ActiveEntityIndex, Result);
	if (arrcap(World->ActiveEntities) > World->EntityCapacity) {
		int32 NewCapacity = arrcap(World->ActiveEntities);
		for (int32 ListIndex = 0; ListIndex < ARRAY_COUNT(World->ComponentLists); ListIndex++) {
			UntypedComponentList* List = &World->ComponentLists[ListIndex];
			List->Indices = ReallocIndices(List->Indices, World->EntityCapacity, NewCapacity);
		}
		int32 LastCapacity = World->EntityCapacity;
		World->EntityCapacity = NewCapacity;

		LogWarning(
			"GameWorld:CreateEntity: Expanding ActiveEntities from %d to %d",
			LastCapacity,
			World->EntityCapacity);
	}
	LogInfo("Create Entity %d[%u:%u]", Result.RawValue, ENTITY_ID_INDEX(Result), ENTITY_ID_GENERATION(Result));
	return Result;
}

int32 _GetNextGeneration(GameWorld* World, EntityId Entity)
{
	const int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	const int32 EntityGeneration = ENTITY_ID_GENERATION(Entity);

	int32 NextGeneration = EntityGeneration + 1;
	if (NextGeneration <= World->AllTimeHighGeneration) {
		if (EntityIndex < World->AllTimeHighGenerationFirstIndex) {
			NextGeneration = World->AllTimeHighGeneration + 1;
			World->AllTimeHighGeneration = NextGeneration;
			World->AllTimeHighGenerationFirstIndex = EntityIndex;
		} else {
			NextGeneration = World->AllTimeHighGeneration;
		}
	} else {
		World->AllTimeHighGeneration = NextGeneration;
		World->AllTimeHighGenerationFirstIndex = EntityIndex;
	}
	return NextGeneration;
}

void DestroyEntity(GameWorld* World, EntityId Entity)
{
	ASSERT(World);
	ASSERT(!World->IsLocked && "Cannot destroy entities while world is locked.");

	ASSERT(EntityIdIsValid(World, Entity));
	LogInfo("Destroy Entity [%d:%d]%u", ENTITY_ID_INDEX(Entity), ENTITY_ID_GENERATION(Entity), Entity.RawValue);

	const int32 EntityIndex = ENTITY_ID_INDEX(Entity);

	arrput(World->AvailableIndexStack, EntityIndex);
	int32 EntityCount = arrlen(World->ActiveEntities);
	const int32 Search = BinarySearch(Entity.RawValue, (int32*)World->ActiveEntities, EntityCount);
	if (Search != NONE) {
		arrdel(World->ActiveEntities, Search);

		// TODO: delete this
		// SDL_memset4(
		// 	&World->ActiveEntities[arrlen(World->ActiveEntities)],
		// 	0xFFFFFFFF,
		// 	arrcap(World->ActiveEntities) - arrlen(World->ActiveEntities));

		World->EntitySignatures[EntityIndex] = (EntitySignature){0};
		World->EntityGenerations[EntityIndex] = _GetNextGeneration(World, Entity);

		for (int32 ListIndex = 0; ListIndex < ARRAY_COUNT(World->ComponentLists); ListIndex++) {
			if (ComponentListHas(&World->ComponentLists[ListIndex], Entity)) {
				ComponentListRemove(&World->ComponentLists[ListIndex], Entity);
			}
		}
	} else {
		LogError("GameWorld:Entities:DestroyEntity: Could not find entity '%d' in ActiveEntities", Entity.RawValue);
		LogError("GameWorld:Entities:DestroyEntity: Dumping Active Entities List:");
		for (int32 Index = 0; Index < EntityCount; Index++) {
			EntityId Entity = World->ActiveEntities[Index];
			LogError(
				"GameWorld:Entities:DestroyEntity: [%d:%d]%u",
				ENTITY_ID_INDEX(Entity),
				ENTITY_ID_GENERATION(Entity),
				Entity.RawValue);
		}
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

	for (int32 Index = 0; Index < arrlen(World->ActiveEntities); Index++) {
		EntityId Entity = World->ActiveEntities[Index];
		int32 EntityIndex = ENTITY_ID_INDEX(Entity);
		EntitySignature Signature = World->EntitySignatures[EntityIndex];
		if (EntitySignaturePassesFilter(Signature, Required, Rejected)) {
			arrput(Entities, Entity);
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

void WorldComponentCounts(GameWorld* World, int32* OutBuffer, int32 BufferLength)
{
	ASSERT(World);
	ASSERT(OutBuffer);
	ASSERT(BufferLength >= ComponentType_Count);

	if (BufferLength < ComponentType_Count) {
		LogError("GameWorld:WorldComponentCounts: Insufficient buffer space to store component counts.");
		return;
	}

	for (int32 Index = 0; Index < ComponentType_Count; Index++) {
		OutBuffer[Index] = World->ComponentLists[Index].Count;
	}
}

void WorldLock(GameWorld* World)
{
	ASSERT(World);
	ASSERT(!World->IsLocked && "Lock/Unlock mismatch");
	World->IsLocked = true;
}

void WorldUnlock(GameWorld* World)
{
	ASSERT(World);
	ASSERT(World->IsLocked && "Lock/Unlock mismatch");
	World->IsLocked = false;
}

bool WorldIsLocked(const GameWorld* World)
{
	return World->IsLocked;
}

typedef struct GameWorldCommandQueue {
	GameWorld* World;
	GameWorldCommand* CommandQueue;
	int32 NextFutureEntityId;
	uint8* ComponentStorage;
	int32 ComponentStorageCapacityBytes;
	int32 ComponentStorageUsedBytes;
} GameWorldCommandQueue;

static void* _CommandQueueAlloc(GameWorldCommandQueue* Queue, int32 Bytes)
{
	ASSERT(
		Queue->ComponentStorageUsedBytes + Bytes < Queue->ComponentStorageCapacityBytes &&
		"Command queue component storage is full, consider increasing capacity in WorldCreateCommandQueue.");

	// TODO: Alignment
	void* Result = Queue->ComponentStorage + Queue->ComponentStorageUsedBytes;
	Queue->ComponentStorageUsedBytes += Bytes;
	return Result;
}

GameWorldCommandQueue* WorldCreateCommandQueue(GameWorld* World)
{
	GameWorldCommandQueue* Queue = (GameWorldCommandQueue*)malloc(sizeof(GameWorldCommandQueue));
	ZERO_STRUCT(Queue);

	Queue->World = World;
	Queue->ComponentStorageCapacityBytes = MEGABYTES(1);
	Queue->ComponentStorage = malloc(Queue->ComponentStorageCapacityBytes);

	ASSERT(Queue->ComponentStorage);

	arrsetcap(Queue->CommandQueue, 128);

	return Queue;
}

void WorldConsumeCommandQueue(GameWorldCommandQueue* Queue)
{
	ASSERT(Queue);
	ASSERT(Queue->World);
	ASSERT(!Queue->World->IsLocked);

	EntityId* FutureEntityMap = NULL;
	arrsetlen(FutureEntityMap, Queue->NextFutureEntityId);

	for (GameWorldCommand* Command = Queue->CommandQueue; Command != arrend(Queue->CommandQueue); Command++) {
		switch (Command->Operation) {
			case GameWorldCommandOp_CreateEntity:
				{
					int32 Index = Command->FutureEntity._Internal;
					ASSERT(Index >= 0 && Index < arrlen(FutureEntityMap));
					ASSERT(FutureEntityMap[Index].RawValue == 0);
					EntityId NewEntity = CreateEntity(Queue->World);
					FutureEntityMap[Index] = NewEntity;
				}
				break;
			case GameWorldCommandOp_DestroyEntity:
				{
					EntityId Entity = Command->Entity;
					int32 Index = Command->FutureEntity._Internal;
					if (Index != NONE) {
						ASSERT(Index >= 0 && Index < arrlen(FutureEntityMap));
						ASSERT(FutureEntityMap[Index].RawValue != 0);
						Entity = FutureEntityMap[Index];
					}
					DestroyEntity(Queue->World, Entity);
				}
				break;
			case GameWorldCommandOp_AddComponent:
				{
					EntityId Entity = Command->Entity;
					int32 Index = Command->FutureEntity._Internal;
					if (Index != NONE) {
						ASSERT(Index >= 0 && Index < arrlen(FutureEntityMap));
						ASSERT(FutureEntityMap[Index].RawValue != 0);
						Entity = FutureEntityMap[Index];
					}
					void* Component = EntityAddComponent(Queue->World, Entity, Command->ComponentType);
					if (Command->ComponentStorage != NULL) {
						int32 ComponentSize = _GetComponentList(Queue->World, Command->ComponentType)->ComponentSize;
						memcpy(Component, Command->ComponentStorage, ComponentSize);
					}
				}
				break;
			case GameWorldCommandOp_RemoveComponent:
				{
					EntityId Entity = Command->Entity;
					int32 Index = Command->FutureEntity._Internal;
					if (Index != NONE) {
						ASSERT(Index >= 0 && Index < arrlen(FutureEntityMap));
						ASSERT(FutureEntityMap[Index].RawValue != 0);
						Entity = FutureEntityMap[Index];
					}
					EntityRemoveComponent(Queue->World, Entity, Command->ComponentType);
				}
				break;
			default:
				ASSERT(false);
				unreachable();
				break;
		}
	}

	WorldDestroyCommandQueue(Queue);
}

void WorldDestroyCommandQueue(GameWorldCommandQueue* Queue)
{
	arrfree(Queue->CommandQueue);
	ZERO_STRUCT(Queue);
	free(Queue);
}

FutureEntityId QueueCreateEntity(GameWorldCommandQueue* Queue)
{
	FutureEntityId FutureEntity = (FutureEntityId){Queue->NextFutureEntityId++};
	GameWorldCommand Command = (GameWorldCommand){
		.Operation = GameWorldCommandOp_CreateEntity,
		.ComponentType = NONE,
		.FutureEntity = FutureEntity,
	};
	arrput(Queue->CommandQueue, Command);
	return FutureEntity;
}

void QueueDestroyEntityId(GameWorldCommandQueue* Queue, EntityId Entity)
{
	GameWorldCommand Command = (GameWorldCommand){
		.Operation = GameWorldCommandOp_DestroyEntity,
		.ComponentType = NONE,
		.Entity = Entity,
		.FutureEntity = (FutureEntityId){NONE},
	};
	arrput(Queue->CommandQueue, Command);
}

void QueueDestroyFutureEntityId(GameWorldCommandQueue* Queue, FutureEntityId FutureEntity)
{
	GameWorldCommand Command = (GameWorldCommand){
		.Operation = GameWorldCommandOp_DestroyEntity,
		.ComponentType = NONE,
		.FutureEntity = FutureEntity,
	};
	arrput(Queue->CommandQueue, Command);
}

// TODO: This is implemented in a weird place, maybe move this to a component specific translation unit
inline const char* ComponentTypeName(ComponentType Type)
{
	return ComponentTypeNames[Type];
}

// Private Implementations
// -------------------------------------------------------
static void* ReallocComponentMemory(void* Memory, int32 ElementSize, int32 LastCapacity, int32 NewCapacity)
{
	Memory = realloc(Memory, NewCapacity * ElementSize);
	ASSERT(Memory);

	if (NewCapacity > LastCapacity) {
		memset((uint8*)Memory + (LastCapacity * ElementSize), 0, (NewCapacity - LastCapacity) * ElementSize);
	}
	return Memory;
}

static EntityId* ReallocEntities(EntityId* Entities, int32 LastCapacity, int32 NewCapacity)
{
	Entities = (EntityId*)realloc(Entities, NewCapacity * sizeof(EntityId));
	ASSERT(Entities);
	if (NewCapacity > LastCapacity) {
		_Static_assert(sizeof(EntityId) % 4 == 0, "memset4 may cause issues now");
		SDL_memset4(Entities + LastCapacity, 0, NewCapacity - LastCapacity);
	}
	return Entities;
}

static int32* ReallocIndices(int32* Indices, int32 LastCapacity, int32 NewCapacity)
{
	Indices = (int32*)realloc(Indices, NewCapacity * sizeof(int32));
	ASSERT(Indices);
	if (NewCapacity > LastCapacity) {
		SDL_memset4(Indices + LastCapacity, (uint32)NONE, NewCapacity - LastCapacity);
	}
	return Indices;
}

static UntypedComponentList CreateComponentList(
	GameWorld* World,
	ComponentType Type,
	int32 ComponentSize,
	int32 Capacity,
	int32 EntityCapacity)
{
	UntypedComponentList List;
	ZERO_STRUCT(&List);
	List.World = World;
	List.Type = Type;
	List.ComponentSize = ComponentSize;
	List.Capacity = Capacity;

	List.ComponentMemory = ReallocComponentMemory(List.ComponentMemory, List.ComponentSize, 0, List.Capacity);
	List.Entities = ReallocEntities(List.Entities, 0, List.Capacity);
	List.Indices = ReallocIndices(List.Indices, 0, EntityCapacity);
	return List;
}

static inline void* ComponentIndexMemory(UntypedComponentList* List, int32 Index)
{
	return ((uint8*)List->ComponentMemory) + (Index * List->ComponentSize);
}

static void DestroyComponentList(UntypedComponentList* List)
{
	free(List->ComponentMemory);
	free(List->Entities);
	free(List->Indices);
	ZERO_STRUCT(List);
}

static void* ComponentListAdd(UntypedComponentList* List, EntityId Entity)
{
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	int32 NewIndex = List->Count;

	ASSERT(EntityIndex <= List->World->EntityCapacity && "Entity has invalid index.");

	ASSERT(List->Indices[EntityIndex] == NONE && "Entity already has component");
	List->Indices[EntityIndex] = NewIndex;

	if (NewIndex == List->Capacity) {
		int32 LastCapacity = List->Capacity;
		List->Capacity *= 2;
		List->ComponentMemory =
			ReallocComponentMemory(List->ComponentMemory, List->ComponentSize, LastCapacity, List->Capacity);
		List->Entities = ReallocEntities(List->Entities, LastCapacity, List->Capacity);

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

	void* RemovedMemory = ComponentIndexMemory(List, RemovedIndex);
	void* LastMemory = ComponentIndexMemory(List, List->Count - 1);
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

	void* Result = ComponentIndexMemory(List, ComponentIndex);
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
	ASSERT(World);
	ASSERT(!World->IsLocked && "Cannot add components while world is locked.");
	ASSERT(EntityIdIsValid(World, Entity));
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	World->EntitySignatures[EntityIndex].RawValue |= BIT_FLAG64(Type);
	LogInfo(
		"GameWorld:EntityAddComponent: added %s component to entity %d[%d]",
		ComponentTypeName(Type),
		ENTITY_ID_INDEX(Entity),
		ENTITY_ID_GENERATION(Entity));
	return ComponentListAdd(_GetComponentList(World, Type), Entity);
}

static void EntityRemoveComponent(GameWorld* World, EntityId Entity, ComponentType Type)
{
	ASSERT(World);
	ASSERT(!World->IsLocked && "Cannot remove components while world is locked.");
	ASSERT(EntityIdIsValid(World, Entity));
	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	World->EntitySignatures[EntityIndex].RawValue &= ~(BIT_FLAG64(Type));
	ComponentListRemove(_GetComponentList(World, Type), Entity);
}

static void* _InternalEntityQueueAddComponent(
	GameWorldCommandQueue* Queue,
	EntityId Entity,
	FutureEntityId FutureEntity,
	ComponentType Type)
{
	ASSERT(Queue);
	ASSERT(Queue->World);
	// Maybe want to move component attributes data (size, etc...) to World level instead of inside the component lists
	size_t ComponentSize = _GetComponentList(Queue->World, Type)->ComponentSize;
	GameWorldCommand Command = (GameWorldCommand){
		.Entity = Entity,
		.FutureEntity = FutureEntity,
		.Operation = GameWorldCommandOp_AddComponent,
		.ComponentType = Type,
		.ComponentStorage = _CommandQueueAlloc(Queue, ComponentSize),
	};
	arrput(Queue->CommandQueue, Command);
	return Command.ComponentStorage;
}

static void* QueueAddComponentEntityId(GameWorldCommandQueue* Queue, EntityId Entity, ComponentType Type)
{
	return _InternalEntityQueueAddComponent(Queue, Entity, (FutureEntityId){NONE}, Type);
}

static void* QueueAddComponentFutureEntityId(GameWorldCommandQueue* Queue, FutureEntityId Entity, ComponentType Type)
{
	ASSERT(Queue);
	ASSERT(Entity._Internal >= 0 && Entity._Internal < Queue->NextFutureEntityId);
	return _InternalEntityQueueAddComponent(Queue, (EntityId){0}, Entity, Type);
}

static void _InternalEntityQueueRemoveComponent(
	GameWorldCommandQueue* Queue,
	EntityId Entity,
	FutureEntityId FutureEntity,
	ComponentType Type)
{
	ASSERT(Queue);
	ASSERT(Queue->World);
	GameWorldCommand Command = (GameWorldCommand){
		.Entity = Entity,
		.FutureEntity = FutureEntity,
		.Operation = GameWorldCommandOp_RemoveComponent,
		.ComponentType = Type,
	};
	arrput(Queue->CommandQueue, Command);
}

static void QueueRemoveComponentEntityId(GameWorldCommandQueue* Queue, EntityId Entity, ComponentType Type)
{
	_InternalEntityQueueRemoveComponent(Queue, Entity, (FutureEntityId){NONE}, Type);
}

static void QueueRemoveComponentFutureEntityId(GameWorldCommandQueue* Queue, FutureEntityId Entity, ComponentType Type)
{
	ASSERT(Queue);
	ASSERT(Entity._Internal >= 0 && Entity._Internal < Queue->NextFutureEntityId);
	_InternalEntityQueueRemoveComponent(Queue, (EntityId){0}, Entity, Type);
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

#define QUEUE_ADD_COMPONENT_IMPLEMENTATION(Type, EID)                                                                  \
	QUEUE_ADD_COMPONENT_PROTOTYPE(Type, EID)                                                                           \
	{                                                                                                                  \
		(COMPONENT_NAME(Type)*)CAT(QueueAddComponent, EID)(Queue, Entity, CAT(ComponentType_, Type));                  \
	}

#define QUEUE_REMOVE_COMPONENT_IMPLEMENTATION(Type, EID)                                                               \
	QUEUE_REMOVE_COMPONENT_PROTOTYPE(Type, EID)                                                                        \
	{                                                                                                                  \
		CAT(QueueRemoveComponent, EID)(Queue, Entity, CAT(ComponentType_, Type));                                      \
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
	QUEUE_ADD_COMPONENT_IMPLEMENTATION(Type, EntityId);                                                                \
	QUEUE_REMOVE_COMPONENT_IMPLEMENTATION(Type, EntityId);                                                             \
	QUEUE_ADD_COMPONENT_IMPLEMENTATION(Type, FutureEntityId);                                                          \
	QUEUE_REMOVE_COMPONENT_IMPLEMENTATION(Type, FutureEntityId);                                                       \
	GET_COMPONENT_IMPLEMENTATION(Type);                                                                                \
	TRYGET_COMPONENT_IMPLEMENTATION(Type);                                                                             \
	HAS_COMPONENT_IMPLEMENTATION(Type);

// For every component type defined in ComponentTypes.h will create corresponding type-safe entity-component interface
// implementations e.g.:
// EntityAddTransformComponent, EntityRemoveTransformComponent, etc...
// _Generic entity-component interface defined in GameWorld.h calls these generated functions
FOR_EACH(COMPONENT_INTERFACE_IMPLEMENTATION, COMPONENT_TYPE_LIST);
