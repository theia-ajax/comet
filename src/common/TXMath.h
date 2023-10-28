#pragma once

#include "Types.h"

#define HANDMADE_FLOAT flt32
#define HANDMADE_MATH_USE_TURNS
#include <HandmadeMath.h>

// ---------------------------------------------------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------------------------------------------------

#define KEpsilonFloat64 0.000000001
#define KEpsilonFloat32 0.00001f
#define KMaxFloat32 FLT_MAX
#define KMaxFloat64 DBL_MAX

// ---------------------------------------------------------------------------------------------------------------------
// Definitions
// ---------------------------------------------------------------------------------------------------------------------

typedef struct Rect {
	int32 X, Y, W, H;
} Rect;

typedef struct Point {
	int32 X, Y;
} Point;

typedef struct Rect16 {
	int16 X, Y, W, H;
} Rect16;

typedef struct Point16 {
	int16 X, Y;
} Point16;

typedef struct RaycastIn {
	Vec2 Start;
	Vec2 End;
	flt32 Fraction;
} RaycastIn;

typedef struct RaycastOut {
	Vec2 Normal;
	flt32 Fraction;
} RaycastOut;

typedef struct AABB {
	Vec2 MinBound;
	Vec2 MaxBound;
} AABB;

typedef struct Circle {
	Vec2 Center;
	flt32 Radius;
} Circle;

enum { KPolygonMaxVerts = 8 };
typedef struct Polygon {
	Vec2 Vertices[KPolygonMaxVerts];
	Vec2 Normals[KPolygonMaxVerts];
	Vec2 Centroid;
	int32 VertexCount;
} Polygon;

// ---------------------------------------------------------------------------------------------------------------------
// Public Interface
// ---------------------------------------------------------------------------------------------------------------------

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
typedef Vec2 Rot2;
Rot2 R2(flt32 Angle);
Rot2 R2Ident(void);
flt32 R2Angle(Rot2 R);
Vec2 R2AxisX(Rot2 R);
Vec2 R2AxisY(Rot2 R);
Vec2 R2Rotate(Rot2 R, Vec2 V);
Vec2 R2InvRotate(Rot2 R, Vec2 V);

// 2D non-scaled transformations
Vec2 TransformV2(Vec2 Point, Rot2 Rotation, Vec2 Translation);
Vec2 InvTransformV2(Vec2 Point, Rot2 Rotation, Vec2 Translation);

/// @param Extents Half-Sizes
AABB AABBCreateCenterExtents(Vec2 Center, Vec2 Extents);
AABB AABBEnvelop(AABB Self, Vec2 Point);
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
bool CircleTestPoint(Vec2 TestPoint, const Circle* CircleShape, Vec2 TxPos, Rot2 TxRot);
bool PolygonTestPoint(Vec2 TestPoint, const Polygon* PolygonShape, Vec2 TxPos, Rot2 TxRot);
void PolygonMakeAABB(Polygon* Self, Vec2 HalfSize);
void PolygonMakeBox(Polygon* Self, Vec2 HalfSize, Vec2 Center, flt32 Angle);
Vec2 ComputeCentroid(const Vec2* Verts, int32 Count);

// ---------------------------------------------------------------------------------------------------------------------
// Inline Implementations
// ---------------------------------------------------------------------------------------------------------------------

// TODO: HandmadeMath.h? Generic ApproxV2/3/4 etc...?
static inline bool Approx(flt32 a, flt32 b)
{
	return fabs(a - b) <= KEpsilonFloat32;
}

// clang-format off
#define SWAP(Type, A, B) do { Type SWAP = A; A = B; B = SWAP; } while (0)
#define SWAP_REF(Type, A, B) SWAP(Type, *A, *B)

#define SWAP_NAME(Name) NAME2(Swap, Name)
#define DEFINE_SWAP(Name, Type) static inline void SWAP_NAME(Name)(Type* A, Type* B) { SWAP_REF(Type, A, B); }
// clang-format on

DEFINE_SWAP(Bool, bool);
DEFINE_SWAP(Int8, int8);
DEFINE_SWAP(Int16, int16);
DEFINE_SWAP(Int32, int32);
DEFINE_SWAP(Int64, int64);
DEFINE_SWAP(UInt8, uint8);
DEFINE_SWAP(UInt16, uint16);
DEFINE_SWAP(UInt32, uint32);
DEFINE_SWAP(UInt64, uint64);
DEFINE_SWAP(Float32, flt32);
DEFINE_SWAP(Float64, flt64);
DEFINE_SWAP(Vec2, Vec2);
DEFINE_SWAP(Vec3, Vec3);
DEFINE_SWAP(Vec4, Vec4);
DEFINE_SWAP(Quat, Quat);
DEFINE_SWAP(Mat2, Mat2);
DEFINE_SWAP(Mat3, Mat3);
DEFINE_SWAP(Mat4, Mat4);

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
