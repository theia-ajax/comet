#pragma once

#include "Math2D.h"
#include "Types.h"

typedef struct TransformComponent {
	Vec2 Position;
	flt32 Rotation;
} TransformComponent;

typedef struct VelocityComponent {
	Vec2 Velocity;
	flt32 AngularVelocity;
} VelocityComponent;

typedef struct SpriteComponent {
	int32 SpriteId;
	flt32 Rotation;
	Vec2 Offset;
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

typedef struct LifetimeComponent {
	flt32 SecondsRemaining;
} LifetimeComponent;

// Add new components here to get component lists added to the gameworld
// Will create component interface, enum value, etc..
#define COMPONENT_TYPES (Transform)(Velocity)(Sprite)(Collider)(Lifetime)

#define COMPONENT_TYPE_LIST CHAIN_COMMA(COMPONENT_TYPES)
#define COMPONENT_TYPE_ENUM_VALUE(CType) CAT(ComponentType_, CType)
#define COMPONENT_TYPE_ENUM_VALUE_ENTRY(CType) COMPONENT_TYPE_ENUM_VALUE(CType) COMMA()
typedef enum ComponentType {
	FOR_EACH(COMPONENT_TYPE_ENUM_VALUE_ENTRY, COMPONENT_TYPE_LIST) ComponentType_Count,
} ComponentType;

_Static_assert(ComponentType_Count <= 64, "More work to be done before more than 64 component types can be supported.");

const char* ComponentTypeName(ComponentType Type);
