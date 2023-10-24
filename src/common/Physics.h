#pragma once

#include "Math.h"

static const uint32 KInvalidHandle;

typedef struct PhysicsObjectHandle {
	uint32 Value;
} PhysicsObjectHandle;

typedef struct PhysicsConstraintHandle {
	uint32 Value;
} PhysicsConstraintHandle;

typedef struct PhysicsObject {
	uint32 Flags;
	Vec2 Position;
	Vec2 LastPosition;
	Vec2 Acceleration;
	real32 Radius;
	real32 Heat;
	uint32 Tint;
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
	real32 TargetDistance;
} PhysLinkConstraint;

typedef struct PhysConstraint {
	PhysConstraintType Type;
	union {
		PhysPinConstraint Pin;
		PhysLinkConstraint Link;
	};
} PhysConstraint;

void VertletObjectUpdate(PhysicsObject* Object, float DeltaTime);
void VertletObjectAccelerate(PhysicsObject* Object, Vec2 Acceleration);

void PhysicsInitialize(void);
void PhysicsShutdown(void);
void PhysicsUpdate(float DeltaTime);
const PhysicsObject* PhysicsGetObjects(void);
size_t PhysicsGetObjectCount(void);
PhysicsObjectHandle PhysicsAddObject(const PhysicsObject* OptionalConfig);
bool PhysObjectHandleIsValid(PhysicsObjectHandle Handle);
PhysicsObject* PhysicsGetObject(PhysicsObjectHandle Handle);
PhysicsConstraintHandle PhysicsAddPinConstraint(PhysicsObjectHandle HObject, Vec2 Position);
PhysicsConstraintHandle PhysicsAddLinkConstraint(
	PhysicsObjectHandle HObject0,
	PhysicsObjectHandle HObject1,
	real32 TargetDistance);
PhysConstraint* PhysicsGetConstraint(PhysicsConstraintHandle Handle);