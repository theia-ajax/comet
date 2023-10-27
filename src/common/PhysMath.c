#include "PhysMath.h"

AABB AABBCreateCenterExtents(Vec2 Center, Vec2 Extents)
{
	return (AABB){
		.MinBound = Sub(Center, Extents),
		.MaxBound = Add(Center, Extents),
	};
}

AABB AABBEnvelop(AABB Self, Vec2 Point)
{
	return (AABB){
		.MinBound = Min(Self.MinBound, Point),
		.MaxBound = Max(Self.MaxBound, Point),
	};
}

bool AABBIsValid(AABB Self)
{
	Vec2 Size = Sub(Self.MaxBound, Self.MinBound);
	bool IsValid = Size.X >= 0.0f && Size.Y >= 0.0f;
	IsValid = IsValid && IsFinite(Self.MinBound) && IsFinite(Self.MaxBound);
	return IsValid;
}

Vec2 AABBCenter(AABB Self)
{
	return Mul(Add(Self.MinBound, Self.MaxBound), 0.5f);
}

Vec2 AABBExtents(AABB Self)
{
	return Mul(Sub(Self.MaxBound, Self.MinBound), 0.5f);
}

real32 AABBPerimeter(AABB Self)
{
	return 2.0f * ((Self.MaxBound.X - Self.MinBound.X) + (Self.MaxBound.Y - Self.MinBound.Y));
}

AABB AABBCombine(AABB A, AABB B)
{
	return (AABB){
		.MinBound = Min(A.MaxBound, B.MinBound),
		.MaxBound = Max(A.MaxBound, B.MaxBound),
	};
}

bool AABBContains(AABB Self, AABB Other)
{
	bool Result = true;
	Result = Result && Self.MinBound.X <= Other.MinBound.X;
	Result = Result && Self.MinBound.Y <= Other.MinBound.Y;
	Result = Result && Self.MaxBound.X >= Other.MaxBound.X;
	Result = Result && Self.MaxBound.Y >= Other.MaxBound.Y;
	return Result;
}

bool AABBTestOverlap(AABB A, AABB B)
{
	Vec2 D1 = Sub(B.MinBound, A.MaxBound);
	Vec2 D2 = Sub(A.MinBound, B.MaxBound);
	bool Result = D1.X <= 0 && D1.Y <= 0 && D2.X <= 0 && D2.Y <= 0;
	return Result;
}

Vec2 R2(real32 Angle)
{
	return (Vec2){
		.X = CosF(Angle),
		.Y = SinF(Angle),
	};
}

Rot2 R2Ident(void)
{
	return (Rot2){0, 1};
}

real32 R2Angle(Rot2 R)
{
	return atan2(R.Y, R.X);
}

Vec2 R2AxisX(Rot2 R)
{
	return (Vec2){R.X, R.Y};
}

Vec2 R2AxisY(Rot2 R)
{
	return (Vec2){-R.Y, R.X};
}

Vec2 R2Rotate(Rot2 R, Vec2 V)
{
	return (Vec2){R.X * V.X - R.Y * V.Y, R.Y * V.X + R.X * V.Y};
}

Vec2 R2InvRotate(Rot2 R, Vec2 V)
{
	return (Vec2){R.X * V.X + R.Y * V.Y, -R.Y * V.X + R.X * V.Y};
}

// Produces scalar equivalent to area of parallelogram formed by A and B
// Equivalent calculation to 2x2 Matrix Determinant
real32 CrossV2(Vec2 A, Vec2 B)
{
	return A.X * B.Y - B.X * A.Y;
}
// Produces a vector perpendicular to A and with magnitude |A|*S
// If S == 1 simply produces perpendicular vector
Vec2 CrossV2F(Vec2 A, real32 S)
{
	return V2(-A.Y * S, A.X * S);
}

Vec2 TransformV2(Vec2 Point, Rot2 Rotation, Vec2 Translation)
{
	return Add(Translation, R2Rotate(Rotation, Point));
}

Vec2 InvTransformV2(Vec2 Point, Rot2 Rotation, Vec2 Translation)
{
	return R2InvRotate(Rotation, Sub(Point, Translation));
}

PhysShapeHandle PhysCreateCircleShape(Vec2 Center, real32 Radius);
PhysShapeHandle PhysCreatePolygonShape(Vec2* Points, size_t PointsCount);
PhysShapeHandle PhysCreateBoxShape(Vec2 HalfSize);

PhysCircleShape* PhysTryGetCircleShape(PhysShapeHandle HShape);
PhysPolygonShape* PhysTryGetPolygonShape(PhysShapeHandle HShape);

bool PhysShapeTestPoint(Vec2 TxPos, PhysShapeHandle HShape, Rot2 TxRot, Vec2 TestPoint)
{
	switch (Shape->Type) {
		case ShapeType_Circle:

		case ShapeType_Box:

			break;

		default:
		case ShapeType_None:
			unreachable();
			break;
	}
}

bool PhysCircleShapeTestPoint(Vec2 TestPoint, const PhysCircleShape* CircleShape, Vec2 TxPos, Rot2 TxRot)
{
	Vec2 TransformedCenter = TransformV2(CircleShape->Center, TxRot, TxPos);
	Vec2 Delta = Sub(TestPoint, TransformedCenter);
	return Dot(Delta, Delta) <= CircleShape->Radius * CircleShape->Radius;
}

bool PhysPolygonShapeTestPoint(Vec2 TestPoint, const PhysPolygonShape* PolygonShape, Vec2 TxPos, Rot2 TxRot)
{
	bool Result = true;
	Vec2 LocalPoint = InvTransformV2(TestPoint, TxRot, TxPos);

	for (int32 VertIndex = 0; VertIndex < PolygonShape->VertexCount; VertIndex++) {
		Vec2 Delta = Sub(LocalPoint, PolygonShape->Vertices[VertIndex]);
		real32 D = Dot(PolygonShape->Normals[VertIndex], Delta);
		if (D > 0.0f) {
			Result = false;
			break;
		}
	}

	return Result;
}