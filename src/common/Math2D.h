#pragma once

#include "Types.h"

#define HANDMADE_FLOAT flt32
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

typedef struct Circle {
	Vec2 Center;
	flt32 Radius;
} Circle;

typedef struct Polygon {
	Vec2 Vertices[KPolygonMaxVerts];
	Vec2 Normals[KPolygonMaxVerts];
	Vec2 Centroid;
	int32 VertexCount;
} Polygon;

typedef struct RaycastIn {
	Vec2 Start;
	Vec2 End;
	flt32 MaxFraction;
} RaycastIn;

typedef struct RaycastOut {
	Vec2 Normal;
	flt32 Fraction;
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
flt32 CrossV2(Vec2 A, Vec2 B);
// Produces a vector perpendicular to A and with magnitude |A|*S
// If S == 1 simply produces perpendicular vector
Vec2 CrossV2F(Vec2 A, flt32 S);

// 2D specific math, maybe find a place for this in HandmadeMath.h at some point.
// These represent 2d rotations as V2(Cos(Angle), Sin(Angle))
Rot2 R2(flt32 Angle);
Rot2 R2Ident(void);
flt32 R2Angle(Rot2 R);
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
flt32 AABBPerimeter(AABB Self);
AABB AABBCombine(AABB A, AABB B);
bool AABBContains(AABB Self, AABB Other);
bool AABBTestOverlap(AABB A, AABB B);
bool AABBRaycast(AABB Self, const RaycastIn* In, RaycastOut* Out);

// Shape Utility Functions
void CircleLocalize(const Circle* Self, Tform2 Transform, Circle* Out);
bool CircleTestPoint(const Circle* Self, Tform2 Transform, Vec2 TestPoint);
AABB CircleCalcAABB(const Circle* Self, Tform2 Transform);
bool CircleRaycast(const Circle* Self, Tform2 Transform, const RaycastIn* In, RaycastOut* Out);
bool CircleIntersectsCircle(const Circle* A, Tform2 TransformA, const Circle* B, Tform2 TransformB);
bool CircleIntersectsPolygon(const Circle* A, Tform2 TransformA, const Polygon* B, Tform2 TransformB);

void PolygonMakeAABB(Polygon* Self, Vec2 HalfSize);
void PolygonMakeBox(Polygon* Self, Vec2 HalfSize, Vec2 Center, flt32 Angle);
void PolygonMakeHull(Polygon* Self, const Vec2* Vertices, int32 VertexCount);
void PolygonLocalize(const Polygon* Self, Tform2 Transform, Polygon* Out);
bool PolygonTestPoint(const Polygon* Self, Tform2 Transform, Vec2 TestPoint);
AABB PolygonCalcAABB(const Polygon* Self, Tform2 Transform);
bool PolygonRaycast(const Polygon* Self, Tform2 Transform, const RaycastIn* In, RaycastOut* Out);
bool PolygonIntersectsCircle(const Polygon* A, Tform2 TransformA, const Circle* B, Tform2 TransformB);
bool PolygonIntersectsPolygon(const Polygon* A, Tform2 TransformA, const Polygon* B, Tform2 TransformB);

Vec2 CalculateCentroid(const Vec2* Verts, int32 Count);

// Inline Implementations
// -------------------------------------------------------
DEFINE_SWAP(Vec2, Vec2);
DEFINE_SWAP(Vec3, Vec3);
DEFINE_SWAP(Vec4, Vec4);
DEFINE_SWAP(Quat, Quat);
DEFINE_SWAP(Mat2, Mat2);
DEFINE_SWAP(Mat3, Mat3);
DEFINE_SWAP(Mat4, Mat4);

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
		flt32: SwapFloat32,                                                                                            \
		flt64: SwapFloat64,                                                                                            \
		Vec2: SwapVec2,                                                                                                \
		Vec3: SwapVec3,                                                                                                \
		Vec4: SwapVec4,                                                                                                \
		Quat: SwapQuat,                                                                                                \
		Mat2: SwapMat2,                                                                                                \
		Mat3: SwapMat3,                                                                                                \
		Mat4: SwapMat4)(&A, &B)
#endif
