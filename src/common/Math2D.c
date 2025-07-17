#include "Math2D.h"
#include "Util.h"

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

AABB AABBInflate(AABB Self, Vec2 HalfAdjust)
{
	return (AABB){
		.MinBound = Sub(Self.MinBound, HalfAdjust),
		.MaxBound = Add(Self.MaxBound, HalfAdjust),
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

float32 AABBPerimeter(AABB Self)
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

	float32 TMin = -INFINITY;
	float32 TMax = INFINITY;
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
		float32 InvDirX = 1.0f / Dir.X;
		float32 T1 = (Self.MinBound.X - Point.X) * InvDirX;
		float32 T2 = (Self.MaxBound.X - Point.X) * InvDirX;
		float32 S = -1.0f;

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
		float32 InvDirY = 1.0f / Dir.Y;
		float32 T1 = (Self.MinBound.Y - Point.Y) * InvDirY;
		float32 T2 = (Self.MaxBound.Y - Point.Y) * InvDirY;
		float32 S = -1.0f;

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

	if (TMin < 0.0f || In->MaxFraction < TMin) {
		return false;
	}

	Out->Fraction = TMin;
	Out->Normal = Normal;
	return true;
}

Vec2 R2(float32 Angle)
{
	return (Vec2){
		.X = CosF(Angle),
		.Y = SinF(Angle),
	};
}

Rot2 R2Ident(void)
{
	return (Rot2){1, 0};
}

float32 R2Angle(Rot2 R)
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

Tform2 T2(Vec2 Position, Rot2 Rotation)
{
	return (Tform2){.Position = Position, .Rotation = Rotation};
}

Tform2 T2Ident(void)
{
	return (Tform2){.Position = V2(0, 0), .Rotation = R2Ident()};
}

// Produces scalar equivalent to area of parallelogram formed by A and B
// Equivalent calculation to 2x2 Matrix Determinant
float32 CrossV2(Vec2 A, Vec2 B)
{
	return A.X * B.Y - B.X * A.Y;
}

// Produces a vector perpendicular to A and with magnitude |A|*S
// If S == 1 simply produces perpendicular vector
Vec2 CrossV2F(Vec2 A, float32 S)
{
	return V2(-A.Y * S, A.X * S);
}

Vec2 TransformV2(Tform2 Transform, Vec2 Point)
{
	return Add(Transform.Position, R2Rotate(Transform.Rotation, Point));
}

Vec2 InvTransformV2(Tform2 Transform, Vec2 Point)
{
	return R2InvRotate(Transform.Rotation, Sub(Point, Transform.Position));
}

void CircleLocalize(const CircleShape* Self, Tform2 Transform, CircleShape* Out)
{
	Out->Center = TransformV2(Transform, Self->Center);
	Out->Radius = Self->Radius;
}

bool CircleTestPoint(const CircleShape* Self, Tform2 Transform, Vec2 TestPoint)
{
	ASSERT(Self);
	Vec2 TransformedCenter = TransformV2(Transform, Self->Center);
	Vec2 Delta = Sub(TestPoint, TransformedCenter);
	return LenSqr(Delta) <= SQUARE(Self->Radius);
}

bool PolygonTestPoint(const PolygonShape* Self, Tform2 Transform, Vec2 TestPoint)
{
	ASSERT(Self);
	bool Result = true;
	Vec2 LocalPoint = InvTransformV2(Transform, TestPoint);

	for (int32 VertIndex = 0; VertIndex < Self->VertexCount; VertIndex++) {
		Vec2 Delta = Sub(LocalPoint, Self->Vertices[VertIndex]);
		float32 D = Dot(Self->Normals[VertIndex], Delta);
		if (D > 0.0f) {
			Result = false;
			break;
		}
	}

	return Result;
}

AABB CircleCalcAABB(const CircleShape* Self, Tform2 Transform)
{
	Vec2 Center = Add(Self->Center, Transform.Position);
	Vec2 Rad2 = V2(Self->Radius, Self->Radius);
	return (AABB){
		.MinBound = Sub(Center, Rad2),
		.MaxBound = Add(Center, Rad2),
	};
}

bool CircleRaycast(const CircleShape* Self, Tform2 Transform, const RaycastIn* In, RaycastOut* Out)
{
	Vec2 Center = TransformV2(Transform, Self->Center);
	Vec2 StartToCenter = Sub(In->Start, Center);
	float32 StartCircDistSqr = LenSqr(StartToCenter) - SQUARE(Self->Radius);

	Vec2 Ray = Sub(In->End, In->Start);
	float32 InvTheta = Dot(StartToCenter, Ray);
	float32 RayLenSqr = LenSqr(Ray);
	float32 Sigma = SQUARE(InvTheta) - RayLenSqr * StartCircDistSqr;

	bool Result = false;

	if (Sigma >= 0.0f && RayLenSqr >= KEpsilonFloat32) {
		float32 IntersectDistSqr = -(InvTheta + SqrtF(Sigma));

		if (IntersectDistSqr >= 0.0f && IntersectDistSqr <= In->MaxFraction * RayLenSqr) {
			Out->Fraction = IntersectDistSqr / RayLenSqr;
			Out->Normal = Norm(Add(StartToCenter, Mul(Ray, Out->Fraction)));
			Result = true;
		}
	}

	return Result;
}

bool CircleIntersectsCircle(const CircleShape* A, Tform2 TransformA, const CircleShape* B, Tform2 TransformB)
{
	Vec2 SelfCenter = TransformV2(TransformA, A->Center);
	Vec2 OtherCenter = TransformV2(TransformB, B->Center);
	float32 DistSqr = LenSqr(Sub(OtherCenter, SelfCenter));
	float32 RadSqr = SQUARE(A->Radius + B->Radius);
	return DistSqr <= RadSqr;
}

bool CircleIntersectsPolygon(const CircleShape* A, Tform2 TransformA, const PolygonShape* B, Tform2 TransformB)
{
	PolygonShape LocalB;
	PolygonLocalize(B, TransformB, &LocalB);

	Vec2 SelfCenter = TransformV2(TransformA, A->Center);

	for (int32 Index = 0; Index < B->VertexCount; Index++) {
		RaycastIn Raycast = (RaycastIn){
			.Start = LocalB.Vertices[Index],
			.End = LocalB.Vertices[(Index + 1) % LocalB.VertexCount],
			.MaxFraction = 1.0f,
		};
		RaycastOut RaycastResult;
		if (CircleRaycast(A, TransformA, &Raycast, &RaycastResult)) {
			return true;
		}
	}

	return false;
}

AABB PolygonCalcAABB(const PolygonShape* Self, Tform2 Transform)
{
	Vec2 Lower = TransformV2(Transform, Self->Vertices[0]);
	Vec2 Upper = Lower;

	for (int32 Index = 1; Index < Self->VertexCount; Index++) {
		Vec2 Vert = TransformV2(Transform, Self->Vertices[Index]);
		Lower = Min(Lower, Vert);
		Upper = Max(Upper, Vert);
	}

	return (AABB){Lower, Upper};
}

Vec2 CalculateCentroid(const Vec2* Verts, int32 Count)
{
	ASSERT(Count >= 3);

	Vec2 Center = V2(0, 0);
	float32 Area = 0.0f;
	Vec2 Base = Verts[0];

	for (int32 Index = 0; Index < Count; Index++) {
		Vec2 E1 = Sub(Verts[Index], Base);
		Vec2 E2 = (Index + 1 < Count) ? Sub(Verts[Index + 1], Base) : V2(0, 0);
		float32 D = CrossV2(E1, E2);
		float32 TriArea = 0.5f * D;
		Area += TriArea;
		Center = Add(Center, Div(Mul(Add(E1, E2), TriArea), 3.0f));
	}

	ASSERT(Area > KEpsilonFloat32);
	Center = Mul(Add(Center, Base), 1.0f / Area);
	return Center;
}

PolygonShape PolygonCreateBox(Vec2 HalfSize, Vec2 Center, float32 Angle)
{
	PolygonShape Result;
	PolygonMakeBox(&Result, HalfSize, Center, Angle);
	return Result;
}

void PolygonMakeAABB(PolygonShape* Self, Vec2 HalfSize)
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

void PolygonMakeBox(PolygonShape* Self, Vec2 HalfSize, Vec2 Center, float32 Angle)
{
	PolygonMakeAABB(Self, HalfSize);
	Self->Centroid = Center;
	Rot2 Rot = R2(Angle);
	for (int32 Index = 0; Index < Self->VertexCount; Index++) {
		Self->Vertices[Index] = TransformV2(T2(Center, Rot), Self->Vertices[Index]);
		Self->Normals[Index] = TransformV2(T2(Center, Rot), Self->Normals[Index]);
	}
}

void PolygonMakeHull(PolygonShape* Self, const Vec2* Vertices, int32 VertexCount)
{
	ASSERT(VertexCount >= 3);

	// For now assume valid hull
	Self->VertexCount = MIN(VertexCount, KPolygonMaxVerts);
	memcpy(Self->Vertices, Vertices, sizeof(Vec2) * Self->VertexCount);
	Self->Centroid = CalculateCentroid(Self->Vertices, Self->VertexCount);
}

void PolygonLocalize(const PolygonShape* Self, Tform2 Transform, PolygonShape* Out)
{
	ZERO_STRUCT(Out);
	Out->Centroid = TransformV2(Transform, Self->Centroid);
	for (int32 Index = 0; Index < Self->VertexCount; Index++) {
		Out->Normals[Index] = R2Rotate(Transform.Rotation, Self->Normals[Index]);
		Out->Vertices[Index] = TransformV2(Transform, Self->Vertices[Index]);
	}
	Out->VertexCount = Self->VertexCount;
}

bool PolygonRaycast(const PolygonShape* Self, Tform2 Transform, const RaycastIn* In, RaycastOut* Out)
{
	// Localize ray to polygon's transform
	Vec2 P0 = InvTransformV2(Transform, In->Start);
	Vec2 P1 = InvTransformV2(Transform, In->End);
	Vec2 Delta = Sub(P1, P0);

	float32 Lower = 0.0f, Upper = In->MaxFraction;
	int32 HitIndex = NONE;

	for (int32 Index = 0; Index < Self->VertexCount; Index++) {
		float32 Numerator = Dot(Self->Normals[Index], Sub(Self->Vertices[Index], P0));
		float32 Denominator = Dot(Self->Normals[Index], Delta);

		if (Denominator == 0) {
			if (Numerator < 0) {
				return false;
			}
		} else {
			if (Denominator < 0 && Numerator < Lower * Denominator) {
				Lower = Numerator / Denominator;
				HitIndex = Index;
			} else if (Denominator > 0 && Numerator < Upper * Denominator) {
				Upper = Numerator / Denominator;
			}
		}

		if (Upper < Lower) {
			return false;
		}
	}

	ASSERT(Lower >= 0.0f && Lower <= In->MaxFraction);

	if (HitIndex != NONE) {
		Out->Fraction = Lower;
		Out->Normal = R2Rotate(Transform.Rotation, Self->Normals[HitIndex]);
		return true;
	}

	return false;
}

bool PolygonIntersectsCircle(const PolygonShape* A, Tform2 TransformA, const CircleShape* B, Tform2 TransformB)
{
	return CircleIntersectsPolygon(B, TransformB, A, TransformA);
}

bool PolygonIntersectsPolygon(const PolygonShape* A, Tform2 TransformA, const PolygonShape* B, Tform2 TransformB)
{
	PolygonShape LocalA, LocalB;
	PolygonLocalize(A, TransformA, &LocalA);
	PolygonLocalize(B, TransformB, &LocalB);

	for (int32 EdgeIndex = 0; EdgeIndex < A->VertexCount + B->VertexCount; EdgeIndex++) {
		Vec2 Normal;
		if (EdgeIndex < A->VertexCount) {
			Normal = LocalA.Normals[EdgeIndex];
		} else {
			Normal = LocalB.Normals[EdgeIndex - A->VertexCount];
		}

		float32 DotA = Dot(LocalA.Vertices[0], Normal);
		float32 MinA = DotA, MaxA = DotA;
		for (int32 Index = 1; Index < A->VertexCount; Index++) {
			DotA = Dot(LocalA.Vertices[Index], Normal);
			MinA = Min(MinA, DotA);
			MaxA = Max(MaxA, DotA);
		}

		float32 DotB = Dot(LocalB.Vertices[0], Normal);
		float32 MinB = DotB, MaxB = DotB;
		for (int32 Index = 1; Index < B->VertexCount; Index++) {
			DotB = Dot(LocalB.Vertices[Index], Normal);
			MinB = Min(MinB, DotB);
			MaxB = Max(MaxB, DotB);
		}

		float32 IntervalDistance = (MinA < MinB) ? MinB - MaxA : MinA - MaxB;
		if (IntervalDistance > 0) {
			return false;
		}
	}

#if 0
	Vec2 RotatedNormals0[KPolygonMaxVerts];
	Vec2 RotatedNormals1[KPolygonMaxVerts];

	for (int32 Index = 0; Index < Self->VertexCount; Index++)
	{
		RotatedNormals0[Index] = R2Rotate(SelfTransform.Rotation, Self->Normals[Index]);
	}

	for (int32 Index = 0; Index < Other->VertexCount; Index++)
	{
		RotatedNormals1[Index] = R2Rotate(OtherTransform.Rotation, Other->Normals[Index]);
	}

	for (int32 Index0 = 0; Index0 < Self->VertexCount; Index0++)
	{
		Vec2 P = TransformV2(SelfTransform, Self->Vertices[Index0]);

		Vec2 Delta = Sub(P, Center1);
		for (int32 Index1 = 0; Index1 < Other->VertexCount; Index1++)
		{
			float32 D = Dot(Delta, RotatedNormals1[Index1]);
			if (D < 0)
			{
				return true;
			}
		}
	}

	return false;
#endif

	return true;
}