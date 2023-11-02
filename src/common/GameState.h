#pragma once

#include "Math2D.h"
#include "StringId.h"

typedef struct EntityId {
	int32 RawValue;
} EntityId;

typedef struct GameState GameState;

typedef struct TransformComponent {
	Vec2 Position;
	flt32 Rotation;
} TransformComponent;

typedef struct ComponentIdSet {
	StringId Transform;
} ComponentIdSet;

extern ComponentIdSet ComponentIds;

GameState* CreateGameState(void);
void DestroyGameState(GameState* Self);

EntityId GameStateCreateEntity(GameState* Self);
void GameStateDestroyEntity(GameState* Self, EntityId Entity);
bool GameStateEntityIdIsValid(GameState* Self, EntityId Entity);

void* EntityAddComponent(GameState* State, EntityId Entity, StringId ComponentId, const void* ComponentData);
void EntityRemoveComponent(GameState* State, EntityId Entity, StringId ComponentId);
void* EntityGetComponent(GameState* State, EntityId Entity, StringId ComponentId);
bool EntityHasComponent(GameState* State, EntityId Entity, StringId ComponentId);

#define AddComponent(State, Entity, Type, Data)                                                                        \
	(NAME2(Type, Component)*)EntityAddComponent(State, Entity, ComponentIds.Type, Data)
#define GetComponent(State, Entity, Type) (NAME2(Type, Component)*)EntityGetComponent(State, Entity, ComponentIds.Type)

// typedef enum ColliderType {
// 	ColliderType_Circle,
// 	ColliderType_Polygon,
// 	ColliderType_Count,
// } ColliderType;

// typedef struct ColliderComponent {
// 	ColliderType Type;
// 	union {
// 		CircleShape Circle;
// 		PolygonShape Polygon;
// 	};
// } ColliderComponent;

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
