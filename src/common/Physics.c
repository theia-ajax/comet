#include "Physics.h"
#include "stb_ds.h"

// Private Defintitions
typedef struct PhysWorld {
	struct {
		PhysCircleShape* Circles;
		PhysPolygonShape* Polygons;
	} Shapes;
} PhysWorld;

// Private Prototypes
static PhysShapeHandle _PhysAllocateShape(PhysWorld* World, PhysShapeType ShapeType);

#define H_SHAPE(TypeId, Index) (((TypeId) << 16) | (Index))
#define H_SHAPE_TYPE(Handle) ((Handle).RawValue >> 16)
#define H_SHAPE_INDEX(Handle) ((Handle).RawValue & 0xFFFF)
#define H_IS_NONE(Handle) ((Handle).RawValue == 0)

// Public Implementations
PhysWorld* PhysCreateWorld(const PhysWorldConfig* Config)
{
	PhysWorld* World = (PhysWorld*)malloc(sizeof(PhysWorld));
	arrsetcap(World->Shapes.Circles, 64);
	arrsetcap(World->Shapes.Polygons, 64);
	return World;
}

void PhysDestroyWorld(PhysWorld* World)
{
	arrfree(World->Shapes.Circles);
	arrfree(World->Shapes.Polygons);
	free(World);
}

PhysShapeHandle PhysCreateCircleShape(PhysWorld* World, Vec2 Center, real32 Radius)
{
	PhysShapeHandle HShape = _PhysAllocateShape(World, PhysShapeType_Circle);
	PhysCircleShape* Circle = PhysTryGetCircleShape(World, HShape);
	if (Circle != NULL) {
		Circle->Center = Center;
		Circle->Radius = Radius;
	}
	return HShape;
}

// Assumes convex hull, validate later
PhysShapeHandle PhysCreatePolygonShape(PhysWorld* World, Vec2* Points, size_t PointsCount)
{
	ASSERT(PointsCount <= KPhysMaxPolygonVerts);
	PhysShapeHandle HShape = _PhysAllocateShape(World, PhysShapeType_Polygon);
	PhysPolygonShape* Polygon = PhysTryGetPolygonShape(World, HShape);
	if (Polygon != NULL) {
		memcpy(Polygon->Vertices, Points, sizeof(*Points) * PointsCount);
		Polygon->VertexCount = MIN(PointsCount, KPhysMaxPolygonVerts);
		for (int32 Index1 = 0; Index1 < Polygon->VertexCount; Index1++) {
			int32 Index2 = (Index1 + 1 < Polygon->VertexCount) ? Index1 + 1 : 0;
			Vec2 Edge = Sub(Polygon->Vertices[Index2], Polygon->Vertices[Index1]);
			Polygon->Normals[Index1] = CrossV2F(Edge, 1.0f);
		}
	}
	return HShape;
}

PhysShapeHandle PhysCreateBoxShape(PhysWorld* World, Vec2 HalfSize)
{
	PhysShapeHandle HShape = _PhysAllocateShape(World, PhysShapeType_Polygon);
	PhysPolygonShape* Box = PhysTryGetPolygonShape(World, HShape);
	if (Box != NULL) {
		Box->Vertices[0] = V2(-HalfSize.X, -HalfSize.Y);
		Box->Vertices[1] = V2(HalfSize.X, -HalfSize.Y);
		Box->Vertices[2] = V2(HalfSize.X, HalfSize.Y);
		Box->Vertices[3] = V2(-HalfSize.X, HalfSize.Y);
		Box->Normals[0] = V2(0, -1);
		Box->Normals[1] = V2(1, 0);
		Box->Normals[2] = V2(0, 1);
		Box->Normals[3] = V2(-1, 0);
		Box->VertexCount = 4;
	}
	return HShape;
}

PhysCircleShape* PhysTryGetCircleShape(PhysWorld* World, PhysShapeHandle HShape)
{
	PhysCircleShape* Result = NULL;
	PhysShapeType ShapeType = H_SHAPE_TYPE(HShape);
	if (ShapeType == PhysShapeType_Circle) {
		int32 Index = H_SHAPE_INDEX(HShape);
		if (VALID_INDEX(Index, arrlen(World->Shapes.Circles))) {
			Result = &World->Shapes.Circles[Index];
		}
	}

	return Result;
}

PhysPolygonShape* PhysTryGetPolygonShape(PhysWorld* World, PhysShapeHandle HShape)
{
	PhysPolygonShape* Result = NULL;
	PhysShapeType ShapeType = H_SHAPE_TYPE(HShape);
	if (ShapeType == PhysShapeType_Polygon) {
		int32 Index = H_SHAPE_INDEX(HShape);
		if (VALID_INDEX(Index, arrlen(World->Shapes.Polygons))) {
			Result = &World->Shapes.Polygons[Index];
		}
	}

	return Result;
}

bool PhysShapeTestPoint(PhysWorld* World, Vec2 TestPoint, PhysShapeHandle HShape, Vec2 TxPos, Rot2 TxRot)
{
	bool Result = false;

	switch (H_SHAPE_TYPE(HShape)) {
		case PhysShapeType_Circle:
			Result = PhysCircleShapeTestPoint(TestPoint, PhysTryGetCircleShape(World, HShape), TxPos, TxRot);
			break;
		case PhysShapeType_Polygon:
			Result = PhysPolygonShapeTestPoint(TestPoint, PhysTryGetPolygonShape(World, HShape), TxPos, TxRot);
			break;
		case PhysShapeType_None:
		default:
			break;
	}

	return Result;
}

bool PhysCircleShapeTestPoint(Vec2 TestPoint, const PhysCircleShape* CircleShape, Vec2 TxPos, Rot2 TxRot)
{
	ASSERT(CircleShape);
	Vec2 TransformedCenter = TransformV2(CircleShape->Center, TxRot, TxPos);
	Vec2 Delta = Sub(TestPoint, TransformedCenter);
	return Dot(Delta, Delta) <= CircleShape->Radius * CircleShape->Radius;
}

bool PhysPolygonShapeTestPoint(Vec2 TestPoint, const PhysPolygonShape* PolygonShape, Vec2 TxPos, Rot2 TxRot)
{
	ASSERT(PolygonShape);
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

// Private implementations

static PhysShapeHandle _PhysAllocateShape(PhysWorld* World, PhysShapeType ShapeType)
{
	PhysShapeHandle Handle = {0};

	ASSERT(VALID_INDEX(ShapeType, PhysShapeType_Count));

	uint32 TypeId = 0;
	uint32 ShapeIndex = 0;

	switch (ShapeType) {
		case PhysShapeType_None:
			break;

		case PhysShapeType_Circle:
			arrput(World->Shapes.Circles, (PhysCircleShape){0});
			ShapeIndex = arrlen(World->Shapes.Circles) - 1;
			break;

		case PhysShapeType_Polygon:
			arrput(World->Shapes.Polygons, (PhysPolygonShape){0});
			ShapeIndex = arrlen(World->Shapes.Polygons) - 1;
			break;

		default:
			unreachable();
	}

	ASSERT((ShapeIndex & 0xFFFF) == ShapeIndex);
	Handle.RawValue = H_SHAPE(TypeId, ShapeIndex);

	return Handle;
}