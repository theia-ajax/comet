#pragma once

#include "Math.h"

typedef struct AABB {
	Vec2 MinBound;
	Vec2 MaxBound;
} AABB;

/// @brief
/// @param Center
/// @param Extents Half-Sizes
/// @return
AABB AABBCreateCenterExtents(Vec2 Center, Vec2 Extents);
AABB AABBEnvelop(AABB Self, Vec2 Point);
bool AABBIsValid(AABB Self);
Vec2 AABBCenter(AABB Self);
Vec2 AABBExtents(AABB Self);
real32 AABBPerimeter(AABB Self);
AABB AABBCombine(AABB A, AABB B);
bool AABBContains(AABB Self, AABB Other);
bool AABBTestOverlap(AABB A, AABB B);

typedef Vec2 Rot2;
Rot2 R2(real32 Angle);
Rot2 R2Ident(void);
real32 R2Angle(Rot2 R);
Vec2 R2AxisX(Rot2 R);
Vec2 R2AxisY(Rot2 R);
Vec2 R2Rotate(Rot2 R, Vec2 V);
Vec2 R2InvRotate(Rot2 R, Vec2 V);

// Produces scalar equivalent to area of parallelogram formed by A and B
// Equivalent calculation to 2x2 Matrix Determinant
real32 CrossV2(Vec2 A, Vec2 B);
// Produces a vector perpendicular to A and with magnitude |A|*S
// If S == 1 simply produces perpendicular vector
Vec2 CrossV2F(Vec2 A, real32 S);

Vec2 TransformV2(Vec2 Point, Rot2 Rotation, Vec2 Translation);
Vec2 InvTransformV2(Vec2 Point, Rot2 Rotation, Vec2 Translation);

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

bool PhysCircleShapeTestPoint(Vec2 TestPoint, const PhysCircleShape* CircleShape, Vec2 TxPos, Rot2 TxRot);
bool PhysPolygonShapeTestPoint(Vec2 TestPoint, const PhysPolygonShape* PolygonShape, Vec2 TxPos, Rot2 TxRot);

//
// Create calls return handles,
PhysShapeHandle PhysCreateCircleShape(Vec2 Center, real32 Radius);
PhysShapeHandle PhysCreatePolygonShape(Vec2* Points, size_t PointsCount);
PhysShapeHandle PhysCreateBoxShape(Vec2 HalfSize);

// TryGet calls return NULL if unable to get the shape. This occurs when:
// - Shape type encoded in handle is not valid for the given function (e.g. getting a circle from a polygon handle)
// - Index encoded in handle is invalid.
PhysCircleShape* PhysTryGetCircleShape(PhysShapeHandle HShape);
PhysPolygonShape* PhysTryGetPolygonShape(PhysShapeHandle HShape);

bool PhysShapeTestPoint(Vec2 TxPos, PhysShapeHandle HShape, Rot2 TxRot, Vec2 TestPoint);