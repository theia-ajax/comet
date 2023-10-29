#pragma once

#include "Math2D.h"

typedef struct PhysWorld PhysWorld;

typedef enum PhysShapeType {
	PhysShapeType_None,
	PhysShapeType_Circle,
	PhysShapeType_Polygon,
	PhysShapeType_Count,
} PhysShapeType;

// PhysShapeHandle encodes an ID and a phys shape type
// Handles are stable
typedef struct PhysShapeHandle {
	uint32 RawValue;
} PhysShapeHandle;

typedef struct PhysWorldConfig {

} PhysWorldConfig;

PhysWorld* PhysCreateWorld(const PhysWorldConfig* Config);
void PhysDestroyWorld(PhysWorld* World);

//
// Create calls return handles,
PhysShapeHandle PhysCreateCircleShape(PhysWorld* World, Vec2 Center, flt32 Radius);
PhysShapeHandle PhysCreatePolygonShape(PhysWorld* World, Vec2* Points, size_t PointsCount);
PhysShapeHandle PhysCreateBoxShape(PhysWorld* World, Vec2 HalfSize);

// TryGet calls return NULL if unable to get the shape. This occurs when:
// - Shape type encoded in handle is not valid for the given function (e.g. getting a circle from a polygon handle)
// - Index encoded in handle is invalid.
Circle* PhysTryGetCircleShape(PhysWorld* World, PhysShapeHandle HShape);
Polygon* PhysTryGetPolygonShape(PhysWorld* World, PhysShapeHandle HShape);
// Get ASSERTs if the handle is invalid
Circle* PhysGetCircleShape(PhysWorld* World, PhysShapeHandle HShape);
Polygon* PhysGetPolygonShape(PhysWorld* World, PhysShapeHandle HShape);

bool PhysShapeTestPoint(PhysWorld* World, PhysShapeHandle HShape, Tform2 Transform, Vec2 TestPoint);
