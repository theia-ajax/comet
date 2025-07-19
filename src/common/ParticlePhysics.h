#pragma once

#include "Math2D.h"

typedef struct SDL_Renderer SDL_Renderer;

static const uint32 KInvalidHandle;

typedef struct PhysicsObjectHandle {
	uint32 Value;
} PhysicsObjectHandle;

typedef struct PhysicsConstraintHandle {
	uint32 Value;
} PhysicsConstraintHandle;

typedef enum PhysicsObjectFlags {
	PhysicsObjectFlags_None = 0,
	PhysicsObjectFlags_OverrideTint = 1 << 0,
} PhysicsObjectFlags;

typedef struct PhysicsObject {
	uint32 Flags;
	Vec2 Position;
	int32 GridCell;
	int32 LastGridCell;
	Vec2 LastPosition;
	Vec2 Acceleration;
	float32 Radius;
	float32 Heat;
	uint32 Tint;
	float32 SecondsAlive;
} PhysicsObject;

typedef enum PhysConstraintType {
	PhysConstraintType_None,
	PhysConstraintType_Pin,
	PhysConstraintType_Link,
	PhysConstraintType_Count,
} PhysConstraintType;

typedef struct PhysPinConstraint {
	PhysicsObjectHandle HObject;
	Vec2 Position;
} PhysPinConstraint;

typedef struct PhysLinkConstraint {
	PhysicsObjectHandle HObject0;
	PhysicsObjectHandle HObject1;
	float32 TargetDistance;
} PhysLinkConstraint;

typedef struct PhysConstraint {
	PhysConstraintType Type;
	union {
		PhysPinConstraint Pin;
		PhysLinkConstraint Link;
	};
} PhysConstraint;

typedef struct PhysicsConfig {
	Vec2 Gravity;
	Vec2 HeatForce;
	float32 HeatTransferRate;
	Vec4 Bounds;
	float32 CellSize;
	int32 MaxPhysicsObjects;
} PhysicsConfig;

void PhysicsInitialize(const PhysicsConfig* Config);
void PhysicsReconfigure(const PhysicsConfig *Config);
void PhysicsShutdown(void);
void PhysicsUpdate(float DeltaTime);
PhysicsConfig PhysicsDefaultConfig(void);

const PhysicsConfig* PhysicsGetConfig(void);
const PhysicsObject* PhysicsGetObjects(void);
PhysicsObject* PhysicsGetObjectsMutable(void);
size_t PhysicsGetObjectCount(void);
void PhysicsClearAllObjects(void);
PhysicsObjectHandle PhysicsAddObject(const PhysicsObject* OptionalConfig);
void PhysicsObjectAccelerate(PhysicsObject* Object, Vec2 Acceleration);
bool PhysObjectHandleIsValid(PhysicsObjectHandle Handle);
PhysicsObject* PhysicsGetObject(PhysicsObjectHandle Handle);
PhysicsConstraintHandle PhysicsAddPinConstraint(PhysicsObjectHandle HObject, Vec2 Position);
PhysicsConstraintHandle PhysicsAddLinkConstraint(
	PhysicsObjectHandle HObject0,
	PhysicsObjectHandle HObject1,
	float32 TargetDistance);
PhysConstraint* PhysicsGetConstraint(PhysicsConstraintHandle Handle);
PhysicsObject* PhysicsGetPinConstraintObject(PhysicsConstraintHandle Handle);
bool PhysicsIsAreaClear(Vec2 Position);

void PhysicsDebugDraw(SDL_Renderer* Renderer);
