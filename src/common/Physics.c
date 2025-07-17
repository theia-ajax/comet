#include "Physics.h"
#include "stb_ds.h"

// Private Defintitions
typedef struct PhysWorld {
	struct {
		CircleShape* Circles;
		PolygonShape* Polygons;
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
	ZERO_STRUCT(World);
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

PhysShapeHandle PhysCreateCircleShape(PhysWorld* World, Vec2 Center, float32 Radius)
{
	PhysShapeHandle HShape = _PhysAllocateShape(World, PhysShapeType_Circle);
	CircleShape* Circle = PhysTryGetCircleShape(World, HShape);
	if (Circle != NULL) {
		Circle->Center = Center;
		Circle->Radius = Radius;
	}
	return HShape;
}

// Assumes convex hull, validate later
PhysShapeHandle PhysCreatePolygonShape(PhysWorld* World, Vec2* Points, size_t PointsCount)
{
	ASSERT(PointsCount <= KPolygonMaxVerts);
	PhysShapeHandle HShape = _PhysAllocateShape(World, PhysShapeType_Polygon);
	PolygonShape* Polygon = PhysTryGetPolygonShape(World, HShape);
	if (Polygon != NULL) {
		memcpy(Polygon->Vertices, Points, sizeof(*Points) * PointsCount);
		Polygon->VertexCount = MIN(PointsCount, KPolygonMaxVerts);
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
	PolygonShape* Box = PhysGetPolygonShape(World, HShape);
	PolygonMakeBox(Box, HalfSize, V2(0, 0), 0);
	return HShape;
}

CircleShape* PhysTryGetCircleShape(PhysWorld* World, PhysShapeHandle HShape)
{
	CircleShape* Result = NULL;
	PhysShapeType ShapeType = H_SHAPE_TYPE(HShape);
	if (ShapeType == PhysShapeType_Circle) {
		int32 Index = H_SHAPE_INDEX(HShape);
		if (VALID_INDEX(Index, arrlen(World->Shapes.Circles))) {
			Result = &World->Shapes.Circles[Index];
		}
	}
	return Result;
}

PolygonShape* PhysTryGetPolygonShape(PhysWorld* World, PhysShapeHandle HShape)
{
	PolygonShape* Result = NULL;
	PhysShapeType ShapeType = H_SHAPE_TYPE(HShape);
	if (ShapeType == PhysShapeType_Polygon) {
		int32 Index = H_SHAPE_INDEX(HShape);
		if (VALID_INDEX(Index, arrlen(World->Shapes.Polygons))) {
			Result = &World->Shapes.Polygons[Index];
		}
	}
	return Result;
}

CircleShape* PhysGetCircleShape(PhysWorld* World, PhysShapeHandle HShape)
{
	PhysShapeType ShapeType = H_SHAPE_TYPE(HShape);
	ASSERT(ShapeType == PhysShapeType_Circle);
	int32 Index = H_SHAPE_INDEX(HShape);
	ASSERT(VALID_INDEX(Index, arrlen(World->Shapes.Circles)));
	return &World->Shapes.Circles[Index];
}

PolygonShape* PhysGetPolygonShape(PhysWorld* World, PhysShapeHandle HShape)
{
	PhysShapeType ShapeType = H_SHAPE_TYPE(HShape);
	ASSERT(ShapeType == PhysShapeType_Polygon);
	int32 Index = H_SHAPE_INDEX(HShape);
	ASSERT(VALID_INDEX(Index, arrlen(World->Shapes.Polygons)));
	return &World->Shapes.Polygons[Index];
}

bool PhysShapeTestPoint(PhysWorld* World, PhysShapeHandle HShape, Tform2 Transform, Vec2 TestPoint)
{
	bool Result = false;

	switch (H_SHAPE_TYPE(HShape)) {
		case PhysShapeType_Circle:
			Result = CircleTestPoint(PhysTryGetCircleShape(World, HShape), Transform, TestPoint);
			break;
		case PhysShapeType_Polygon:
			Result = PolygonTestPoint(PhysTryGetPolygonShape(World, HShape), Transform, TestPoint);
			break;
		case PhysShapeType_None:
		default: break;
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
		case PhysShapeType_None: break;

		case PhysShapeType_Circle:
			arrput(World->Shapes.Circles, (CircleShape){0});
			ShapeIndex = arrlen(World->Shapes.Circles) - 1;
			break;

		case PhysShapeType_Polygon:
			arrput(World->Shapes.Polygons, (PolygonShape){0});
			ShapeIndex = arrlen(World->Shapes.Polygons) - 1;
			break;

		default: unreachable();
	}

	ASSERT((ShapeIndex & 0xFFFF) == ShapeIndex);
	Handle.RawValue = H_SHAPE(TypeId, ShapeIndex);

	return Handle;
}