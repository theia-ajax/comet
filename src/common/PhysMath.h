#pragma once

#include "Math.h"

typedef struct RaycastIn {
	Vec2 Start;
	Vec2 End;
	real32 Fraction;
} RaycastIn;

typedef struct RaycastOut {
	Vec2 Normal;
	real32 Fraction;
} RaycastOut;

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
AABB AABBTranslate(AABB Self, Vec2 Translation);
bool AABBIsValid(AABB Self);
Vec2 AABBCenter(AABB Self);
Vec2 AABBExtents(AABB Self);
real32 AABBPerimeter(AABB Self);
AABB AABBCombine(AABB A, AABB B);
bool AABBContains(AABB Self, AABB Other);
bool AABBTestOverlap(AABB A, AABB B);
bool AABBRaycast(AABB Self, const RaycastIn* In, RaycastOut* Out);

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
