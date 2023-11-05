#pragma once

#include "ComponentTypes.h"
#include "Math2D.h"
#include "Types.h"

typedef struct EntityId {
	int32 RawValue;
} EntityId;

typedef struct EntitySignature {
	uint64 RawValue;
} EntitySignature;

typedef struct GameWorld GameWorld;

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

// Generic Component Interface
// Defines Add, Remove, Get, Has for each type of component and provides a _Generic macro for each action.
// -------------------------------------------------------
#define COMPONENT_NAME(Type) CAT(Type, Component)
#define COMPONENT_FUNC_NAME(Func, Type) CAT(Func, COMPONENT_NAME(Type))
#define COMPONENT_ADD_NAME(Type) COMPONENT_FUNC_NAME(EntityAdd, Type)
#define COMPONENT_REMOVE_NAME(Type) COMPONENT_FUNC_NAME(EntityRemove, Type)
#define COMPONENT_GET_NAME(Type) COMPONENT_FUNC_NAME(EntityGet, Type)
#define COMPONENT_TRYGET_NAME(Type) COMPONENT_FUNC_NAME(EntityTryGet, Type)
#define COMPONENT_HAS_NAME(Type) COMPONENT_FUNC_NAME(EntityHas, Type)
#define COMPONENT_HAS(Type) CAT(EntityHas, Type)

#define ADD_COMPONENT_PROTOTYPE(Type)                                                                                  \
	COMPONENT_NAME(Type) *                                                                                             \
		COMPONENT_ADD_NAME(Type)(GameWorld * World, EntityId Entity)

#define REMOVE_COMPONENT_PROTOTYPE(Type) void COMPONENT_REMOVE_NAME(Type)(GameWorld * World, EntityId Entity)

#define GET_COMPONENT_PROTOTYPE(Type)                                                                                  \
	COMPONENT_NAME(Type) * COMPONENT_GET_NAME(Type)(GameWorld * World, EntityId Entity)
#define TRYGET_COMPONENT_PROTOTYPE(Type)                                                                                  \
	COMPONENT_NAME(Type) * COMPONENT_TRYGET_NAME(Type)(GameWorld * World, EntityId Entity)

#define HAS_COMPONENT_PROTOTYPE(Type) bool COMPONENT_HAS_NAME(Type)(GameWorld * World, EntityId Entity)

#define DECLARE_COMPONENT_INTERFACE(Type)                                                                              \
	ADD_COMPONENT_PROTOTYPE(Type);                                                                                     \
	REMOVE_COMPONENT_PROTOTYPE(Type);                                                                                  \
	GET_COMPONENT_PROTOTYPE(Type);                                                                                     \
	TRYGET_COMPONENT_PROTOTYPE(Type);                                                                                     \
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

#define GetComponent(Component, World, Entity)                                                                         \
	_Generic(((Component){0})COMPONENT_GET_GENERIC_ENTRIES(COMPONENT_TYPE_LIST))((World), (Entity))

#define TryGetComponent(Component, World, Entity)                                                                         \
	_Generic(((Component){0})COMPONENT_TRYGET_GENERIC_ENTRIES(COMPONENT_TYPE_LIST))((World), (Entity))

#define HasComponent(Component, World, Entity)                                                                         \
	_Generic(((Component){0})COMPONENT_HAS_GENERIC_ENTRIES(COMPONENT_TYPE_LIST))((World), (Entity))

#define HAS_COMPONENTS(World, Entity, ...) \
	__VA_OPT__(EXPAND(HAS_COMPONENTS_HELPER(World, Entity, __VA_ARGS__)))
#define HAS_COMPONENTS_HELPER(World, Entity, First, ...) \
	COMPONENT_HAS(First)(World, Entity) \
	__VA_OPT__(&& HAS_COMPONENTS_AGAIN PARENS (World, Entity, __VA_ARGS__))
#define HAS_COMPONENTS_AGAIN() HAS_COMPONENTS_HELPER

#define SIGNATURE(...) \
	(EntitySignature){__VA_OPT__(EXPAND(SIGNATURE_HELPER(__VA_ARGS__)))}
#define SIGNATURE_HELPER(First, ...) \
	COMPONENT_TYPE_ENUM_VALUE(First) \
	__VA_OPT__(| SIGNATURE_AGAIN PARENS (__VA_ARGS__))
#define SIGNATURE_AGAIN() SIGNATURE_HELPER

#define REQUIRED(...) \
	(EntitySignature){__VA_OPT__(EXPAND(REQUIRED_HELPER(__VA_ARGS__)))}
#define REQUIRED_HELPER(First, ...) \
	BIT_FLAG64(COMPONENT_TYPE_ENUM_VALUE(First)) \
	__VA_OPT__(| REQUIRED_AGAIN PARENS (__VA_ARGS__))
#define REQUIRED_AGAIN() REQUIRED_HELPER

#define REJECTED(...) \
	(EntitySignature){~(0 __VA_OPT__(| EXPAND(REJECTED_HELPER(__VA_ARGS__))))}
#define REJECTED_HELPER(First, ...) \
	BIT_FLAG64(COMPONENT_TYPE_ENUM_VALUE(First)) \
	__VA_OPT__(| REJECTED_AGAIN PARENS (__VA_ARGS__))
#define REJECTED_AGAIN() REJECTED_HELPER
