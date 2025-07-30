#pragma once

#include "Entity.h"
#include "Math2D.h"
#include "SpriteDatabase.h"
#include "StringId.h"

// Not really a hard limit atm just seems useful to keep track of bloated components
enum { KMaxComponentSizeInBytes = 256 };

typedef struct NameComponent {
	StringId NameId;
} NameComponent;

typedef struct ChildOfComponent {
	EntityId Parent;
} ChildOfComponent;

typedef struct TransformComponent {
	Vec2 Position;
	float32 Rotation;
} TransformComponent;

typedef struct LocalTransformComponent {
	float32 LocalRotation;
	Vec2 LocalPosition;
} LocalTransformComponent;

typedef struct VelocityComponent {
	Vec2 Velocity;
	float32 AngularVelocity;
} VelocityComponent;

typedef struct SpriteComponent {
	SpriteId SpriteId;
	float32 Rotation;
	Vec2 Offset;
} SpriteComponent;

typedef struct SpriteTilesComponent {
	Point Tiles;
} SpriteTilesComponent;

typedef struct RenderLayerComponent {
	int32 Layer;
} RenderLayerComponent;

typedef struct RenderTintComponent {
	Vec4 TintColor;
} RenderTintComponent;

typedef enum ColliderType {
	ColliderType_Circle,
	ColliderType_Polygon,
	ColliderType_Count,
} ColliderType;

typedef struct ColliderComponent {
	ColliderType Type;
	int32 Group; // Colliders in the same group won't intersect, more advanced filtering later if necessary
	union {
		CircleShape Circle;
		PolygonShape Polygon;
	};
} ColliderComponent;

typedef struct LifetimeComponent {
	float32 SecondsRemaining;
} LifetimeComponent;

enum { KMaxSensorEntities = 8 };
typedef struct SensorComponent {
	FixedArray(EntityId, KMaxSensorEntities) Entities;
} SensorComponent;

typedef struct HitReceiverComponent {
	EntityId SourceEntity;
	int32 LastHitPriority;
} HitReceiverComponent;

typedef struct DamageSourceComponent {
	float32 DamageAmount;
} DamageSourceComponent;

typedef struct DamageReceiverComponent {
	float32 DamageAccumulator;
} DamageReceiverComponent;

typedef struct DurabilityComponent {
	float32 CurrentDurability;
} DurabilityComponent;

typedef struct TimerComponent {
	float32 SecondsElapsed;
} TimerComponent;

typedef struct BehaviorComponent {
	int32 State;
	int32 SubState;
} BehaviorComponent;

// TODO: Move these------------------------------
bool ColliderIntersectsCollider(
	const ColliderComponent* A,
	Tform2 TransformA,
	const ColliderComponent* B,
	Tform2 TransformB);

Tform2 T2Component(const TransformComponent* Transform);
// -----------------------------------------------

// clang-format off
// Add new components here to get component lists added to the gameworld
// Will create component interface, enum value, etc..
#define COMPONENT_TYPE_LIST                                                                                            \
	Name, ChildOf, Transform, LocalTransform, Velocity, Sprite, SpriteTiles, RenderLayer, RenderTint, Lifetime,        \
		Collider, DamageSource, DamageReceiver, Durability, Timer, Behavior

#define COMPONENT_TYPE_ENUM_VALUE(CType) CAT(ComponentType_, CType)
#define COMPONENT_TYPE_ENUM_VALUE_ENTRY(CType) COMPONENT_TYPE_ENUM_VALUE(CType) COMMA()
typedef enum ComponentType {
	FOR_EACH(COMPONENT_TYPE_ENUM_VALUE_ENTRY, COMPONENT_TYPE_LIST)
	ComponentType_Count,
} ComponentType;

_Static_assert(ComponentType_Count <= 64, "More work to be done before more than 64 component types can be supported.");

#define COMPONENT_SIZE_ENUM_ENTRY(Type) CAT(CAT(K, CAT(Type, Component)), Size) = sizeof(CAT(Type, Component)),

enum { FOR_EACH(COMPONENT_SIZE_ENUM_ENTRY, COMPONENT_TYPE_LIST) };

#define VALIDATE_COMPONENT_TYPE(Type)                                                                                  \
	_Static_assert(                                                                                                    \
		sizeof(CAT(Type, Component)) <= KMaxComponentSizeInBytes,                                                      \
		"Size of " #Type "Component is greater than KMaxComponentSizeInBytes");

FOR_EACH(VALIDATE_COMPONENT_TYPE, COMPONENT_TYPE_LIST);

const char* ComponentTypeName(ComponentType Type);
bool ComponentTypeTryParse(const char *TypeString, int32 TypeStringLength, ComponentType *OutType);
