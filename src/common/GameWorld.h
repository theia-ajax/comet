#pragma once

#include "Components.h"
#include "Entity.h"
#include "Math2D.h"
#include "Types.h"

typedef struct GameWorld GameWorld;
typedef struct GameWorldCommandQueue GameWorldCommandQueue;

GameWorld* CreateGameWorld(void);
void DestroyGameWorld(GameWorld* World);

EntityId CreateEntity(GameWorld* World);
void DestroyEntity(GameWorld* World, EntityId Entity);
bool EntityIdIsValid(GameWorld* World, EntityId Entity);
EntitySignature EntityGetSignature(GameWorld* World, EntityId Entity);
bool EntitySignaturePassesFilter(EntitySignature Signature, EntitySignature Required, EntitySignature Rejected);
EntityId* WorldEntitiesBegin(GameWorld* World);
EntityId* WorldEntitiesEnd(GameWorld* World);
EntityId* WorldQueryEntities(GameWorld* World, EntitySignature Required, EntitySignature Rejected);
void WorldQueryFree(EntityId* Query);
int32 WorldEntityCount(GameWorld* World);
// BufferLength is required to be at least the same size as ComponentType_Count
void WorldComponentCounts(GameWorld* World, int32* OutBuffer, int32 BufferLength);

void WorldLock(GameWorld* World);
void WorldUnlock(GameWorld* World);
bool WorldIsLocked(const GameWorld* World);

typedef struct FutureEntityId {
	int32 _Internal;
} FutureEntityId;
#define KInvalidFutureEntityId ((FutureEntityId){NONE})

// The command queue is allocated and needs to be freed by either consuming it via WorldConsumeCommandQueue which will
// execute all of the commands or by calling WorldDestroyCommandQueue to free the allocation but not execute the
// commands.
GameWorldCommandQueue* WorldCreateCommandQueue(GameWorld* World);
void WorldConsumeCommandQueue(GameWorldCommandQueue* Queue);
void WorldDestroyCommandQueue(GameWorldCommandQueue* Queue);
FutureEntityId QueueCreateEntity(GameWorldCommandQueue* Queue);
void QueueDestroyEntityId(GameWorldCommandQueue* Queue, EntityId Entity);
void QueueDestroyFutureEntityId(GameWorldCommandQueue* Queue, FutureEntityId FutureEntity);
#define QueueDestroyEntity(Queue, Entity)                                                                              \
	_Generic((Entity), EntityId: QueueDestroyEntityId, FutureEntityId: QueueDestroyFutureEntityId)(Queue, Entity)

// T* QueueAddComponent(GameWorldCommandQueue* Queue, Entity)

void WorldDeferQueueBegin(GameWorld* World);
void WorldFlushDeferQueue(GameWorld* World);
void WorldDeferQueueEnd(GameWorld* World);

// Generic Component Interface
// Defines Add, Remove, Get, Has for each type of component and provides a _Generic macro for each action.
// -------------------------------------------------------
#define COMPONENT_NAME(Type) CAT(Type, Component)
#define COMPONENT_FUNC_NAME(Func, Type) CAT(Func, COMPONENT_NAME(Type))
#define COMPONENT_ADD_NAME(Type) COMPONENT_FUNC_NAME(EntityAdd, Type)
#define COMPONENT_QUEUE_ADD_NAME(Type, EID) CAT(COMPONENT_FUNC_NAME(QueueEntityAdd, Type), EID)
#define COMPONENT_REMOVE_NAME(Type) COMPONENT_FUNC_NAME(EntityRemove, Type)
#define COMPONENT_QUEUE_REMOVE_NAME(Type, EID) CAT(COMPONENT_FUNC_NAME(QueueEntityRemove, Type), EID)
#define COMPONENT_GET_NAME(Type) COMPONENT_FUNC_NAME(EntityGet, Type)
#define COMPONENT_TRYGET_NAME(Type) COMPONENT_FUNC_NAME(EntityTryGet, Type)
#define COMPONENT_HAS_NAME(Type) COMPONENT_FUNC_NAME(EntityHas, Type)
#define COMPONENT_HAS(Type) CAT(EntityHas, Type)

#define ADD_COMPONENT_PROTOTYPE(Type)                                                                                  \
	COMPONENT_NAME(Type) * COMPONENT_ADD_NAME(Type)(GameWorld * World, EntityId Entity)

#define REMOVE_COMPONENT_PROTOTYPE(Type) void COMPONENT_REMOVE_NAME(Type)(GameWorld * World, EntityId Entity)

#define QUEUE_ADD_COMPONENT_PROTOTYPE(Type, EID)                                                                       \
	COMPONENT_NAME(Type) * COMPONENT_QUEUE_ADD_NAME(Type, EID)(GameWorldCommandQueue * Queue, EID Entity)

#define QUEUE_REMOVE_COMPONENT_PROTOTYPE(Type, EID)                                                                    \
	void COMPONENT_QUEUE_REMOVE_NAME(Type, EID)(GameWorldCommandQueue * Queue, EID Entity)

#define GET_COMPONENT_PROTOTYPE(Type)                                                                                  \
	COMPONENT_NAME(Type) * COMPONENT_GET_NAME(Type)(GameWorld * World, EntityId Entity)
#define TRYGET_COMPONENT_PROTOTYPE(Type)                                                                               \
	COMPONENT_NAME(Type) * COMPONENT_TRYGET_NAME(Type)(GameWorld * World, EntityId Entity)

#define HAS_COMPONENT_PROTOTYPE(Type) bool COMPONENT_HAS_NAME(Type)(GameWorld * World, EntityId Entity)

#define DECLARE_COMPONENT_INTERFACE(Type)                                                                              \
	ADD_COMPONENT_PROTOTYPE(Type);                                                                                     \
	REMOVE_COMPONENT_PROTOTYPE(Type);                                                                                  \
	QUEUE_ADD_COMPONENT_PROTOTYPE(Type, EntityId);                                                                     \
	QUEUE_REMOVE_COMPONENT_PROTOTYPE(Type, EntityId);                                                                  \
	QUEUE_ADD_COMPONENT_PROTOTYPE(Type, FutureEntityId);                                                               \
	QUEUE_REMOVE_COMPONENT_PROTOTYPE(Type, FutureEntityId);                                                            \
	GET_COMPONENT_PROTOTYPE(Type);                                                                                     \
	TRYGET_COMPONENT_PROTOTYPE(Type);                                                                                  \
	HAS_COMPONENT_PROTOTYPE(Type);

#define DECLARE_COMPONENT_INTERFACES(First, ...)                                                                       \
	DECLARE_COMPONENT_INTERFACE(First);                                                                                \
	DECLARE_COMPONENT_INTERFACES(__VA_ARGS__)

FOR_EACH(DECLARE_COMPONENT_INTERFACE, COMPONENT_TYPE_LIST);

// _Generic setups for Add/Remove/Get/Has
#define COMPONENT_ADD_GENERIC_ENTRY(Type) , COMPONENT_NAME(Type) : COMPONENT_ADD_NAME(Type)
#define COMPONENT_ADD_GENERIC_ENTRIES(...) FOR_EACH(COMPONENT_ADD_GENERIC_ENTRY, __VA_ARGS__)

#define COMPONENT_REMOVE_GENERIC_ENTRY(Type) , COMPONENT_NAME(Type) : COMPONENT_REMOVE_NAME(Type)
#define COMPONENT_REMOVE_GENERIC_ENTRIES(...) FOR_EACH(COMPONENT_REMOVE_GENERIC_ENTRY, __VA_ARGS__)

#define COMPONENT_QUEUE_ADD_GENERIC_ENTRY(Type, EID) , COMPONENT_NAME(Type) : COMPONENT_QUEUE_ADD_NAME(Type, EID)
#define COMPONENT_QUEUE_ADD_GENERIC_ENTITY_ID_ENTRY(Type) COMPONENT_QUEUE_ADD_GENERIC_ENTRY(Type, EntityId)
#define COMPONENT_QUEUE_ADD_GENERIC_FUTURE_ENTITY_ID_ENTRY(Type) COMPONENT_QUEUE_ADD_GENERIC_ENTRY(Type, FutureEntityId)
#define COMPONENT_QUEUE_ADD_ENTITY_ID_GENERIC_ENTRIES(...)                                                             \
	FOR_EACH(COMPONENT_QUEUE_ADD_GENERIC_ENTITY_ID_ENTRY, __VA_ARGS__)
#define COMPONENT_QUEUE_ADD_FUTURE_ENTITY_ID_GENERIC_ENTRIES(...)                                                      \
	FOR_EACH(COMPONENT_QUEUE_ADD_GENERIC_FUTURE_ENTITY_ID_ENTRY, __VA_ARGS__)

#define COMPONENT_QUEUE_REMOVE_GENERIC_ENTRY(Type, EID) , COMPONENT_NAME(Type) : COMPONENT_QUEUE_REMOVE_NAME(Type, EID)
#define COMPONENT_QUEUE_REMOVE_GENERIC_ENTITY_ID_ENTRY(Type) COMPONENT_QUEUE_REMOVE_GENERIC_ENTRY(Type, EntityId)
#define COMPONENT_QUEUE_REMOVE_GENERIC_FUTURE_ENTITY_ID_ENTRY(Type)                                                    \
	COMPONENT_QUEUE_REMOVE_GENERIC_ENTRY(Type, FutureEntityId)
#define COMPONENT_QUEUE_REMOVE_ENTITY_ID_GENERIC_ENTRIES(...)                                                          \
	FOR_EACH(COMPONENT_QUEUE_REMOVE_GENERIC_ENTITY_ID_ENTRY, __VA_ARGS__)
#define COMPONENT_QUEUE_REMOVE_FUTURE_ENTITY_ID_GENERIC_ENTRIES(...)                                                   \
	FOR_EACH(COMPONENT_QUEUE_REMOVE_GENERIC_FUTURE_ENTITY_ID_ENTRY, __VA_ARGS__)

#define COMPONENT_GET_GENERIC_ENTRY(Type) , COMPONENT_NAME(Type) : COMPONENT_GET_NAME(Type)
#define COMPONENT_GET_GENERIC_ENTRIES(...) FOR_EACH(COMPONENT_GET_GENERIC_ENTRY, __VA_ARGS__)

#define COMPONENT_TRYGET_GENERIC_ENTRY(Type) , COMPONENT_NAME(Type) : COMPONENT_TRYGET_NAME(Type)
#define COMPONENT_TRYGET_GENERIC_ENTRIES(...) FOR_EACH(COMPONENT_TRYGET_GENERIC_ENTRY, __VA_ARGS__)

#define COMPONENT_HAS_GENERIC_ENTRY(Type) , COMPONENT_NAME(Type) : COMPONENT_HAS_NAME(Type)
#define COMPONENT_HAS_GENERIC_ENTRIES(...) FOR_EACH(COMPONENT_HAS_GENERIC_ENTRY, __VA_ARGS__)

#define AddComponent(Component, World, Entity)                                                                         \
	_Generic(((Component){0})COMPONENT_ADD_GENERIC_ENTRIES(COMPONENT_TYPE_LIST))((World), (Entity))

#define RemoveComponent(Component, World, Entity)                                                                      \
	_Generic(((Component){0})COMPONENT_REMOVE_GENERIC_ENTRIES(COMPONENT_TYPE_LIST))((World), (Entity))

// _Generic(((Component){0})COMPONENT_QUEUE_ADD_GENERIC_ENTRIES(COMPONENT_TYPE_LIST))((Queue), (Entity))
#define QueueAddComponent(Component, Queue, Entity)                                                                    \
	_Generic(                                                                                                          \
		(Entity),                                                                                                      \
		FutureEntityId: _Generic(((Component){0})COMPONENT_QUEUE_ADD_FUTURE_ENTITY_ID_GENERIC_ENTRIES(                 \
			COMPONENT_TYPE_LIST)),                                                                                     \
		EntityId: _Generic(((Component){0})COMPONENT_QUEUE_ADD_ENTITY_ID_GENERIC_ENTRIES(COMPONENT_TYPE_LIST)))(       \
		(Queue),                                                                                                       \
		(Entity))

#define QueueRemoveComponent(Component, Queue, Entity)                                                                 \
	_Generic(                                                                                                          \
		(Entity),                                                                                                      \
		FutureEntityId: _Generic(((Component){0})COMPONENT_QUEUE_REMOVE_FUTURE_ENTITY_ID_GENERIC_ENTRIES(              \
			COMPONENT_TYPE_LIST)),                                                                                     \
		EntityId: _Generic(((Component){0})COMPONENT_QUEUE_REMOVE_ENTITY_ID_GENERIC_ENTRIES(COMPONENT_TYPE_LIST)))(    \
		(Queue),                                                                                                       \
		(Entity))

#define GetComponent(Component, World, Entity)                                                                         \
	_Generic(((Component){0})COMPONENT_GET_GENERIC_ENTRIES(COMPONENT_TYPE_LIST))((World), (Entity))

#define TryGetComponent(Component, World, Entity)                                                                      \
	_Generic(((Component){0})COMPONENT_TRYGET_GENERIC_ENTRIES(COMPONENT_TYPE_LIST))((World), (Entity))

#define HasComponent(Component, World, Entity)                                                                         \
	_Generic(((Component){0})COMPONENT_HAS_GENERIC_ENTRIES(COMPONENT_TYPE_LIST))((World), (Entity))

#define GetOrAddComponent(Component, World, Entity)                                                                    \
	(HasComponent(Component, World, Entity) ? GetComponent(Component, World, Entity)                                   \
											: AddComponent(Component, World, Entity))

#define HAS_COMPONENTS(World, Entity, ...) __VA_OPT__(EXPAND(HAS_COMPONENTS_HELPER(World, Entity, __VA_ARGS__)))
#define HAS_COMPONENTS_HELPER(World, Entity, First, ...)                                                               \
	COMPONENT_HAS(First)(World, Entity) __VA_OPT__(&&HAS_COMPONENTS_AGAIN PARENS(World, Entity, __VA_ARGS__))
#define HAS_COMPONENTS_AGAIN() HAS_COMPONENTS_HELPER

#define SIGNATURE(...)                                                                                                 \
	(EntitySignature)                                                                                                  \
	{                                                                                                                  \
		__VA_OPT__(EXPAND(SIGNATURE_HELPER(__VA_ARGS__)))                                                              \
	}
#define SIGNATURE_HELPER(First, ...)                                                                                   \
	COMPONENT_TYPE_ENUM_VALUE(First)                                                                                   \
	__VA_OPT__(| SIGNATURE_AGAIN PARENS(__VA_ARGS__))
#define SIGNATURE_AGAIN() SIGNATURE_HELPER

#define REQUIRED(...)                                                                                                  \
	(EntitySignature)                                                                                                  \
	{                                                                                                                  \
		__VA_OPT__(EXPAND(REQUIRED_HELPER(__VA_ARGS__)))                                                               \
	}
#define REQUIRED_HELPER(First, ...)                                                                                    \
	BIT_FLAG64(COMPONENT_TYPE_ENUM_VALUE(First))                                                                       \
	__VA_OPT__(| REQUIRED_AGAIN PARENS(__VA_ARGS__))
#define REQUIRED_AGAIN() REQUIRED_HELPER

#define REJECTED(...)                                                                                                  \
	(EntitySignature)                                                                                                  \
	{                                                                                                                  \
		~(0 __VA_OPT__(| EXPAND(REJECTED_HELPER(__VA_ARGS__))))                                                        \
	}
#define REJECTED_HELPER(First, ...)                                                                                    \
	BIT_FLAG64(COMPONENT_TYPE_ENUM_VALUE(First))                                                                       \
	__VA_OPT__(| REJECTED_AGAIN PARENS(__VA_ARGS__))
#define REJECTED_AGAIN() REJECTED_HELPER
