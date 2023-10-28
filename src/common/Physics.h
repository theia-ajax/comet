#pragma once

#include "PhysMath.h"

typedef struct PhysWorld PhysWorld;

typedef enum PhysShapeType {
	PhysShapeType_None,
	PhysShapeType_Circle,
	PhysShapeType_Polygon,
	PhysShapeType_Count,
} PhysShapeType;

typedef struct PhysCircleShape {
	Vec2 Center;
	real32 Radius;
} PhysCircleShape;

enum { KPhysMaxPolygonVerts = 8 };
typedef struct PhysPolygonShape {
	Vec2 Vertices[KPhysMaxPolygonVerts];
	Vec2 Normals[KPhysMaxPolygonVerts];
	int32 VertexCount;
} PhysPolygonShape;

// PhysShapeHandle encodes an ID and a phys shape type
// Handles are stable
typedef struct PhysShapeHandle {
	uint32 RawValue;
} PhysShapeHandle;

typedef struct PhysWorldConfig {

} PhysWorldConfig;


PhysWorld* PhysCreateWorld(const PhysWorldConfig* Config);
void PhysDestroyWorld(PhysWorld* World);

bool PhysCircleShapeTestPoint(Vec2 TestPoint, const PhysCircleShape* CircleShape, Vec2 TxPos, Rot2 TxRot);
bool PhysPolygonShapeTestPoint(Vec2 TestPoint, const PhysPolygonShape* PolygonShape, Vec2 TxPos, Rot2 TxRot);
//
// Create calls return handles,
PhysShapeHandle PhysCreateCircleShape(PhysWorld* World, Vec2 Center, real32 Radius);
PhysShapeHandle PhysCreatePolygonShape(PhysWorld* World, Vec2* Points, size_t PointsCount);
PhysShapeHandle PhysCreateBoxShape(PhysWorld* World, Vec2 HalfSize);

// TryGet calls return NULL if unable to get the shape. This occurs when:
// - Shape type encoded in handle is not valid for the given function (e.g. getting a circle from a polygon handle)
// - Index encoded in handle is invalid.
PhysCircleShape* PhysTryGetCircleShape(PhysWorld* World, PhysShapeHandle HShape);
PhysPolygonShape* PhysTryGetPolygonShape(PhysWorld* World, PhysShapeHandle HShape);

bool PhysShapeTestPoint(PhysWorld* World, Vec2 TestPoint, PhysShapeHandle HShape, Vec2 TxPos, Rot2 TxRot);
