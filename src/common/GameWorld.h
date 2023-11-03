#pragma once

#include "Math2D.h"
#include "StringId.h"

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

#define COMPONENT_NAME(Type) NAME2(Type, Component)
#define COMPONENT_FUNC_NAME(Func, Type) NAME2(Func, COMPONENT_NAME(Type))
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

#define DEFINE_COMPONENT_INTERFACE(Type)                                                                               \
	ADD_COMPONENT_PROTOTYPE(Type);                                                                                     \
	REMOVE_COMPONENT_PROTOTYPE(Type);                                                                                  \
	GET_COMPONENT_PROTOTYPE(Type);                                                                                     \
	HAS_COMPONENT_PROTOTYPE(Type);

#define COMPONENT_GENERIC_ENTRY(Func, Type) COMPONENT_NAME(Type) : COMPONENT_FUNC_NAME(Func, Type)

// clang-format off
#define COMPONENT_GENERIC_ENTRIES(Func) \
	COMPONENT_GENERIC_ENTRY(Func, Transform), \
	COMPONENT_GENERIC_ENTRY(Func, Sprite), \
	COMPONENT_GENERIC_ENTRY(Func, Collider)

DEFINE_COMPONENT_INTERFACE(Transform);
DEFINE_COMPONENT_INTERFACE(Sprite);
DEFINE_COMPONENT_INTERFACE(Collider);
// clang-format on

#define AddComponent(Component, World, Entity)                                                                              \
	_Generic(((Component){0}), COMPONENT_GENERIC_ENTRIES(EntityAdd))((World), (Entity), NULL)

#define RemoveComponent(Component, World, Entity)                                                                      \
	_Generic(((Component){0}), COMPONENT_GENERIC_ENTRIES(EntityRemove))((World), (Entity))

#define GetComponent(Component, World, Entity)                                                                         \
	_Generic(((Component){0}), COMPONENT_GENERIC_ENTRIES(EntityGet))((World), (Entity))

#define HasComponent(Component, World, Entity)                                                                         \
	_Generic(((Component){0}), COMPONENT_GENERIC_ENTRIES(EntityHas))((World), (Entity))

// typedef struct SpriteComponent {
// 	int32 SpriteId;
// 	Vec4 TintColor;
// } SpriteComponent;

// typedef struct GameEntity {
// 	Vec2 Position;
// 	flt32 Rotation;
// 	ColliderComponent Collider;
// 	SpriteComponent Sprite;
// } GameEntity;
