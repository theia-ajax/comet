#pragma once

#include "Math.h"

static const uint32 KInvalidHandle;

typedef struct PhysicsObjectHandle {
	uint32 Value;
} PhysicsObjectHandle;

typedef struct PhysicsObject {
	uint32 Flags;
	Vec2 Position;
	Vec2 LastPosition;
	Vec2 Acceleration;
	real32 Radius;
	real32 Heat;
	uint32 Tint;
} PhysicsObject;

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
void PhysicsAddPinConstraint(PhysicsObjectHandle HObject, Vec2 Position);
void PhysicsAddLinkConstraint(
	PhysicsObjectHandle HObject0,
	PhysicsObjectHandle HObject1,
	real32 TargetDistance);