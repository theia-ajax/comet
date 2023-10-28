#include "Math2D.h"

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

AABB AABBTranslate(AABB Self, Vec2 Translation)
{
	return (AABB){
		.MinBound = Add(Self.MinBound, Translation),
		.MaxBound = Add(Self.MaxBound, Translation),
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

flt32 AABBPerimeter(AABB Self)
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

bool AABBRaycast(AABB Self, const RaycastIn* In, RaycastOut* Out)
{
	ASSERT(In != NULL);
	ASSERT(Out != NULL);

	flt32 TMin = -KMaxFloat32;
	flt32 TMax = KMaxFloat32;
	Vec2 Normal = V2(0, 0);
	ZERO_STRUCT(Out);

	Vec2 Point = In->Start;
	Vec2 Dir = Sub(In->End, In->Start);
	Vec2 AbsDir = Abs(Dir);

	if (AbsDir.X < KEpsilonFloat32) {
		if (Point.X < Self.MinBound.X || Point.X > Self.MaxBound.X) {
			return false;
		}
	} else {
		flt32 InvDirX = 1.0f / Dir.X;
		flt32 T1 = (Self.MinBound.X - Point.X) * InvDirX;
		flt32 T2 = (Self.MaxBound.X - Point.X) * InvDirX;
		flt32 S = -1.0f;

		if (T1 > T2) {
			Swap(T1, T2);
			S = 1.0f;
		}

		if (T1 > TMin) {
			Normal.X = S;
		}

		TMax = Min(TMax, T2);
		if (TMin > TMax) {
			return false;
		}
	}

	Vec3 Test1 = V3(1, 2, 3);
	Vec3 Test2 = V3(4, 5, 6);
	Swap(Test1, Test2);

	if (AbsDir.Y < KEpsilonFloat32) {
		if (Point.Y < Self.MinBound.Y || Point.Y > Self.MaxBound.Y) {
			return false;
		}
	} else {
		flt32 InvDirY = 1.0f / Dir.Y;
		flt32 T1 = (Self.MinBound.Y - Point.Y) * InvDirY;
		flt32 T2 = (Self.MaxBound.Y - Point.Y) * InvDirY;
		flt32 S = -1.0f;

		if (T1 > T2) {
			Swap(T1, T2);
			S = 1.0f;
		}

		if (T1 > TMin) {
			Normal.X = 0;
			Normal.Y = S;
		}

		TMax = Min(TMax, T2);
		if (TMin > TMax) {
			return false;
		}
	}

	if (TMin < 0.0f || In->Fraction < TMin) {
		return false;
	}

	Out->Fraction = TMin;
	Out->Normal = Normal;
	return true;
}

Vec2 R2(flt32 Angle)
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

flt32 R2Angle(Rot2 R)
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
flt32 CrossV2(Vec2 A, Vec2 B)
{
	return A.X * B.Y - B.X * A.Y;
}

// Produces a vector perpendicular to A and with magnitude |A|*S
// If S == 1 simply produces perpendicular vector
Vec2 CrossV2F(Vec2 A, flt32 S)
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


bool CircleTestPoint(Vec2 TestPoint, const Circle* CircleShape, Vec2 TxPos, Rot2 TxRot)
{
	ASSERT(CircleShape);
	Vec2 TransformedCenter = TransformV2(CircleShape->Center, TxRot, TxPos);
	Vec2 Delta = Sub(TestPoint, TransformedCenter);
	return Dot(Delta, Delta) <= CircleShape->Radius * CircleShape->Radius;
}

bool PolygonTestPoint(Vec2 TestPoint, const Polygon* PolygonShape, Vec2 TxPos, Rot2 TxRot)
{
	ASSERT(PolygonShape);
	bool Result = true;
	Vec2 LocalPoint = InvTransformV2(TestPoint, TxRot, TxPos);

	for (int32 VertIndex = 0; VertIndex < PolygonShape->VertexCount; VertIndex++) {
		Vec2 Delta = Sub(LocalPoint, PolygonShape->Vertices[VertIndex]);
		flt32 D = Dot(PolygonShape->Normals[VertIndex], Delta);
		if (D > 0.0f) {
			Result = false;
			break;
		}
	}

	return Result;
}

Vec2 ComputeCentroid(const Vec2* Verts, int32 Count)
{
	ASSERT(Count >= 3);

	Vec2 Center = V2(0, 0);
	flt32 Area = 0.0f;
	Vec2 Base = Verts[0];
	
	for (int32 Index = 0; Index < Count; Index++)
	{
		Vec2 E1 = Sub(Verts[Index], Base);
		Vec2 E2 = (Index + 1 < Count) ? Sub(Verts[Index + 1], Base) : V2(0, 0);
		flt32 D = CrossV2(E1, E2);
		flt32 TriArea = 0.5f * D;
		Area += TriArea;
		Center = Add(Center, Div(Mul(Add(E1, E2), TriArea), 3.0f));
	}

	ASSERT(Area > KEpsilonFloat32);
	Center = Mul(Add(Center, Base), 1.0f / Area);
	return Center;
}

void PolygonMakeAABB(Polygon* Self, Vec2 HalfSize)
{
	Self->VertexCount = 4;
	Self->Vertices[0] = V2(-HalfSize.X, -HalfSize.Y);
	Self->Vertices[1] = V2(HalfSize.X, -HalfSize.Y);
	Self->Vertices[2] = V2(HalfSize.X, HalfSize.Y);
	Self->Vertices[3] = V2(-HalfSize.X, HalfSize.Y);
	Self->Normals[0] = V2(0, -1);
	Self->Normals[1] = V2(1, 0);
	Self->Normals[2] = V2(0, 1);
	Self->Normals[3] = V2(-1, 0);
	Self->Centroid = V2(0, 0);
}

void PolygonMakeBox(Polygon* Self, Vec2 HalfSize, Vec2 Center, flt32 Angle)
{
	PolygonMakeAABB(Self, HalfSize);
	Self->Centroid = Center;
	Rot2 Rot = R2(Angle);
	for (int32 Index = 0; Index < Self->VertexCount; Index++)
	{
		Self->Vertices[Index] = TransformV2(Self->Vertices[Index], Rot, Center);
		Self->Normals[Index] = TransformV2(Self->Normals[Index], Rot, Center);
	}
}
