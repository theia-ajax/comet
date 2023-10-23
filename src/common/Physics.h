#pragma once

#include "Math.h"

typedef struct VertletObject {
	Vec2 Position;
	Vec2 LastPosition;
	Vec2 Acceleration;
	real32 Radius;
	real32 Heat;
	uint8 R, G, B, A;
} VertletObject;

void VertletObjectUpdate(VertletObject* Object, float DeltaTime);
void VertletObjectAccelerate(VertletObject* Object, Vec2 Acceleration);

void PhysicsInitialize(void);
void PhysicsShutdown(void);
void PhysicsUpdate(float DeltaTime);
const VertletObject* PhysicsGetObjects(void);
size_t PhysicsGetObjectCount(void);
VertletObject* PhysicsAddObject(const VertletObject* OptionalConfig);