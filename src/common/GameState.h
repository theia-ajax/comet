#pragma once

#include "Math2D.h"

typedef struct EntityId {
	int32 RawValue;
} EntityId;

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

typedef struct SpriteComponent {
	int32 SpriteId;
	Vec4 TintColor;
} SpriteComponent;

typedef struct GameEntity {
	Vec2 Position;
	flt32 Rotation;
	ColliderComponent Collider;
	SpriteComponent Sprite;
} GameEntity;

typedef struct GameState GameState;

GameState* CreateGameState(void);
void DestroyGameState(GameState* Self);

EntityId GameStateCreateEntity(GameState* Self);
void GameStateDestroyEntity(GameState* Self, EntityId Entity);
bool GameStateEntityIdIsValid(GameState* Self, EntityId Entity);
// GameEntity* GameStateTryGetEntity(GameState* Self, EntityId Entity);
// GameEntity* GameStateGetEntity(GameState* Self, EntityId Entity);
GameEntity* GameStateEntityList(GameState* Self);
int32 GameStateEntityCount(GameState* Self);