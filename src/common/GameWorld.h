#pragma once

#include "Math2D.h"
#include "StringId.h"
#include "Types.h"

#define COMPONENT_TYPES (Transform)(Sprite)(Collider)
#define COMPONENT_TYPE_LIST CHAIN_COMMA(COMPONENT_TYPES)

typedef struct EntityId {
	int32 RawValue;
} EntityId;

typedef struct GameWorld GameWorld;

typedef struct TransformComponent {
	Vec2 Position;
	flt32 Rotation;
} TransformComponent;

typedef struct SpriteComponent {
	int32 SpriteId;
} SpriteComponent;

typedef enum ColliderType {
	ColliderType_Circle,
	ColliderType_Polygon,
	ColliderType_Count,
} ColliderType;

typedef struct ColliderComponent {
	ColliderType Type;
	union {
		CircleShape Circle;
		PolygonShape Polygon;
	};
} ColliderComponent;

GameWorld* CreateGameWorld(void);
void DestroyGameWorld(GameWorld* World);

EntityId CreateEntity(GameWorld* World);
void DestroyEntity(GameWorld* World, EntityId Entity);
bool EntityIdIsValid(GameWorld* World, EntityId Entity);

// Generic Component Interface
// Defines Add, Remove, Get, Has for each type of component and provides a _Generic macro for each action.
// -------------------------------------------------------
#define COMPONENT_NAME(Type) CAT(Type, Component)
#define COMPONENT_FUNC_NAME(Func, Type) CAT(Func, COMPONENT_NAME(Type))
#define COMPONENT_ADD_NAME(Type) COMPONENT_FUNC_NAME(EntityAdd, Type)
#define COMPONENT_REMOVE_NAME(Type) COMPONENT_FUNC_NAME(EntityRemove, Type)
#define COMPONENT_GET_NAME(Type) COMPONENT_FUNC_NAME(EntityGet, Type)
#define COMPONENT_HAS_NAME(Type) COMPONENT_FUNC_NAME(EntityHas, Type)

#define ADD_COMPONENT_PROTOTYPE(Type)                                                                                  \
	COMPONENT_NAME(Type) *                                                                                             \
		COMPONENT_ADD_NAME(Type)(GameWorld * World, EntityId Entity, const COMPONENT_NAME(Type) * ComponentData)

#define REMOVE_COMPONENT_PROTOTYPE(Type) void COMPONENT_REMOVE_NAME(Type)(GameWorld * World, EntityId Entity)

#define GET_COMPONENT_PROTOTYPE(Type)                                                                                  \
	COMPONENT_NAME(Type) * COMPONENT_GET_NAME(Type)(GameWorld * World, EntityId Entity)

#define HAS_COMPONENT_PROTOTYPE(Type) bool COMPONENT_HAS_NAME(Type)(GameWorld * World, EntityId Entity)

#define DECLARE_COMPONENT_INTERFACE(Type)                                                                               \
	ADD_COMPONENT_PROTOTYPE(Type);                                                                                     \
	REMOVE_COMPONENT_PROTOTYPE(Type);                                                                                  \
	GET_COMPONENT_PROTOTYPE(Type);                                                                                     \
	HAS_COMPONENT_PROTOTYPE(Type);

#define DECLARE_COMPONENT_INTERFACES(First, ...)                                                                        \
	DECLARE_COMPONENT_INTERFACE(First);                                                                                 \
	DECLARE_COMPONENT_INTERFACES(__VA_ARGS__)

FOR_EACH(DECLARE_COMPONENT_INTERFACE, COMPONENT_TYPE_LIST);

// _Generic setups for Add/Remove/Get/Has
#define COMPONENT_ADD_GENERIC_ENTRY(Type) , COMPONENT_NAME(Type) : COMPONENT_ADD_NAME(Type)
#define COMPONENT_ADD_GENERIC_ENTRIES(...) FOR_EACH(COMPONENT_ADD_GENERIC_ENTRY, __VA_ARGS__)

#define COMPONENT_REMOVE_GENERIC_ENTRY(Type) , COMPONENT_NAME(Type) : COMPONENT_REMOVE_NAME(Type)
#define COMPONENT_REMOVE_GENERIC_ENTRIES(...) FOR_EACH(COMPONENT_REMOVE_GENERIC_ENTRY, __VA_ARGS__)

#define COMPONENT_GET_GENERIC_ENTRY(Type) , COMPONENT_NAME(Type) : COMPONENT_GET_NAME(Type)
#define COMPONENT_GET_GENERIC_ENTRIES(...) FOR_EACH(COMPONENT_GET_GENERIC_ENTRY, __VA_ARGS__)

#define COMPONENT_HAS_GENERIC_ENTRY(Type) , COMPONENT_NAME(Type) : COMPONENT_HAS_NAME(Type)
#define COMPONENT_HAS_GENERIC_ENTRIES(...) FOR_EACH(COMPONENT_HAS_GENERIC_ENTRY, __VA_ARGS__)

#define AddComponent(Component, World, Entity)                                                                         \
	_Generic(((Component){0})COMPONENT_ADD_GENERIC_ENTRIES(COMPONENT_TYPE_LIST))((World), (Entity), NULL)

#define RemoveComponent(Component, World, Entity)                                                                      \
	_Generic(((Component){0})COMPONENT_REMOVE_GENERIC_ENTRIES(COMPONENT_TYPE_LIST))((World), (Entity))

#define GetComponent(Component, World, Entity)                                                                         \
	_Generic(((Component){0})COMPONENT_GET_GENERIC_ENTRIES(COMPONENT_TYPE_LIST))((World), (Entity))

#define HasComponent(Component, World, Entity)                                                                         \
	_Generic(((Component){0})COMPONENT_HAS_GENERIC_ENTRIES(COMPONENT_TYPE_LIST))((World), (Entity))
