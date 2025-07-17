#pragma once

#include "Types.h"

#define HANDMADE_FLOAT float32
#define HANDMADE_MATH_USE_TURNS
#include <HandmadeMath.h>

// Constants
// -------------------------------------------------------
enum { KPolygonMaxVerts = 8 };

// Public Definitions
// -------------------------------------------------------

// Not sure where to put these guys really
// clang-format off
typedef struct Rect { int32 X, Y, W, H; } Rect;
typedef struct Point { int32 X, Y; } Point;
typedef struct Rect16 {	int16 X, Y, W, H; } Rect16;
typedef struct Point16 { int16 X, Y; } Point16;
// clang-format on

// Shapes
typedef struct AABB {
	Vec2 MinBound;
	Vec2 MaxBound;
} AABB;

typedef struct CircleShape {
	Vec2 Center;
	float32 Radius;
} CircleShape;

typedef struct PolygonShape {
	Vec2 Vertices[KPolygonMaxVerts];
	Vec2 Normals[KPolygonMaxVerts];
	Vec2 Centroid;
	int32 VertexCount;
} PolygonShape;

typedef struct RaycastIn {
	Vec2 Start;
	Vec2 End;
	float32 MaxFraction;
} RaycastIn;

typedef struct RaycastOut {
	Vec2 Normal;
	float32 Fraction;
} RaycastOut;

typedef Vec2 Rot2;
typedef struct Tform2 {
	Vec2 Position;
	Rot2 Rotation;
} Tform2;

// Public Interface
// -------------------------------------------------------

// TODO: These should probably make their way into HandmadeMath.h at some point with some genericized
// version of Cross()
// Produces scalar equivalent to area of parallelogram formed by A and B
// Equivalent calculation to 2x2 Matrix Determinant
float32 CrossV2(Vec2 A, Vec2 B);
// Produces a vector perpendicular to A and with magnitude |A|*S
// If S == 1 simply produces perpendicular vector
Vec2 CrossV2F(Vec2 A, float32 S);

// 2D specific math, maybe find a place for this in HandmadeMath.h at some point.
// These represent 2d rotations as V2(Cos(Angle), Sin(Angle))
Rot2 R2(float32 Angle);
Rot2 R2Ident(void);
float32 R2Angle(Rot2 R);
Vec2 R2AxisX(Rot2 R);
Vec2 R2AxisY(Rot2 R);
Vec2 R2Rotate(Rot2 R, Vec2 V);
Vec2 R2InvRotate(Rot2 R, Vec2 V);

Tform2 T2(Vec2 Position, Rot2 Rotation);
Tform2 T2Ident(void);

// 2D non-scaled transformations
Vec2 TransformV2(Tform2 Transform, Vec2 Point);
Vec2 InvTransformV2(Tform2 Transform, Vec2 Point);

/// @param Extents Half-Sizes
AABB AABBCreateCenterExtents(Vec2 Center, Vec2 Extents);
AABB AABBEnvelop(AABB Self, Vec2 Point);
AABB AABBInflate(AABB Self, Vec2 HalfAdjust);
AABB AABBTranslate(AABB Self, Vec2 Translation);
bool AABBIsValid(AABB Self);
Vec2 AABBCenter(AABB Self);
Vec2 AABBExtents(AABB Self);
float32 AABBPerimeter(AABB Self);
AABB AABBCombine(AABB A, AABB B);
bool AABBContains(AABB Self, AABB Other);
bool AABBTestOverlap(AABB A, AABB B);
bool AABBRaycast(AABB Self, const RaycastIn* In, RaycastOut* Out);

// Shape Utility Functions
void CircleLocalize(const CircleShape* Self, Tform2 Transform, CircleShape* Out);
bool CircleTestPoint(const CircleShape* Self, Tform2 Transform, Vec2 TestPoint);
AABB CircleCalcAABB(const CircleShape* Self, Tform2 Transform);
bool CircleRaycast(const CircleShape* Self, Tform2 Transform, const RaycastIn* In, RaycastOut* Out);
bool CircleIntersectsCircle(const CircleShape* A, Tform2 TransformA, const CircleShape* B, Tform2 TransformB);
bool CircleIntersectsPolygon(const CircleShape* A, Tform2 TransformA, const PolygonShape* B, Tform2 TransformB);

PolygonShape PolygonCreateBox(Vec2 HalfSize, Vec2 Center, float32 Angle);
void PolygonMakeAABB(PolygonShape* Self, Vec2 HalfSize);
void PolygonMakeBox(PolygonShape* Self, Vec2 HalfSize, Vec2 Center, float32 Angle);
void PolygonMakeHull(PolygonShape* Self, const Vec2* Vertices, int32 VertexCount);
void PolygonLocalize(const PolygonShape* Self, Tform2 Transform, PolygonShape* Out);
bool PolygonTestPoint(const PolygonShape* Self, Tform2 Transform, Vec2 TestPoint);
AABB PolygonCalcAABB(const PolygonShape* Self, Tform2 Transform);
bool PolygonRaycast(const PolygonShape* Self, Tform2 Transform, const RaycastIn* In, RaycastOut* Out);
bool PolygonIntersectsCircle(const PolygonShape* A, Tform2 TransformA, const CircleShape* B, Tform2 TransformB);
bool PolygonIntersectsPolygon(const PolygonShape* A, Tform2 TransformA, const PolygonShape* B, Tform2 TransformB);

Vec2 CalculateCentroid(const Vec2* Verts, int32 Count);

// Inline Implementations
// -------------------------------------------------------

#ifdef Swap
// Allows Types.h Swap to exist without needing to know about vector math types
// Means code that doesn't care about vector math doesn't need to include this file to get Swap support on primtives.
#undef Swap
#define Swap(A, B)                                                                                                     \
	_Generic(                                                                                                          \
		(A),                                                                                                           \
		int8: SwapInt8,                                                                                                \
		int16: SwapInt16,                                                                                              \
		int32: SwapInt32,                                                                                              \
		int64: SwapInt64,                                                                                              \
		uint8: SwapUInt8,                                                                                              \
		uint16: SwapUInt16,                                                                                            \
		uint32: SwapUInt32,                                                                                            \
		uint64: SwapUInt64,                                                                                            \
		float32: SwapFloat32,                                                                                          \
		float64: SwapFloat64,                                                                                          \
		Vec2: SwapVec2,                                                                                                \
		Vec3: SwapVec3,                                                                                                \
		Vec4: SwapVec4,                                                                                                \
		Quat: SwapQuat,                                                                                                \
		Mat2: SwapMat2,                                                                                                \
		Mat3: SwapMat3,                                                                                                \
		Mat4: SwapMat4)(&A, &B)
#endif
