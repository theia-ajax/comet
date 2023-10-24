#include "Physics.h"

#include <stb_ds.h>

// Constants
const Vec2 KGravity = (Vec2){0.0f, 1000.0f};
const Vec2 KHeatForce = (Vec2){0.0f, -3000.0f};

// Private Definitions
struct {
	PhysicsObject* Objects;
	PhysConstraint* Constraints;
} GPhysics;

// Private Prototypes
static void _PhysicsUpdateVertletObjects(float DeltaTime);
static void _PhysicsApplyConstraints(float DeltaTime);
static void _PhysicsApplyGravity(void);
static void _PhysicsApplyHeat(void);
static void _PhysicsSolveCollisions(void);
// static void _PhysicsApplyConstraint(PhysConstraint* Constraint);

// Public Implementations
void VertletObjectUpdate(PhysicsObject* Object, float DeltaTime)
{
	Vec2 Velocity = Sub(Object->Position, Object->LastPosition);
	Object->LastPosition = Object->Position;
	Object->Position =
		Add(Add(Object->Position, Velocity), Mul(Object->Acceleration, DeltaTime * DeltaTime));
	Object->Heat -= (Object->Heat * 1.0f) * DeltaTime;

	real32 HeaterDistance = 80.0f;
	real32 HeaterThreshold = 1080 - HeaterDistance;

	if (Object->Position.Y > HeaterThreshold) {
		real32 DeltaY = (Object->Position.Y - HeaterThreshold) / HeaterDistance;
		real32 Rate = Lerp(0.0f, 20.0f, DeltaY * DeltaY);
		Object->Heat += Rate * DeltaTime;
	} else if (Object->Position.Y < 24) {
		// Object->Heat -= 2.0f * DeltaTime;
	}
	Object->Heat = Clamp(Object->Heat, 0, 1);
	ZERO_STRUCT(&Object->Acceleration);
}

void VertletObjectAccelerate(PhysicsObject* Object, Vec2 Acceleration)
{
	Object->Acceleration = Add(Object->Acceleration, Acceleration);
}

void PhysicsInitialize(void)
{
	arrsetcap(GPhysics.Objects, 1024);
	arrsetcap(GPhysics.Constraints, 1024);
}

void PhysicsShutdown(void)
{
	arrfree(GPhysics.Objects);
	arrfree(GPhysics.Constraints);
}

void PhysicsUpdate(float DeltaTime)
{
	enum { KSubSteps = 8 };
	real32 SubDeltaTime = DeltaTime / KSubSteps;

	for (int32 SubStepIndex = 0; SubStepIndex < KSubSteps; SubStepIndex++) {
		_PhysicsApplyGravity();
		_PhysicsApplyHeat();
		_PhysicsSolveCollisions();
		_PhysicsUpdateVertletObjects(SubDeltaTime);
		_PhysicsApplyConstraints(DeltaTime);
	}
}

const PhysicsObject* PhysicsGetObjects(void)
{
	return GPhysics.Objects;
}

PhysicsConstraintHandle PhysicsAddPinConstraint(PhysicsObjectHandle HObject, Vec2 Position)
{
	PhysConstraint Constraint = (PhysConstraint){
		.Type = PhysConstraintType_Pin,
		.Pin =
			(PhysPinConstraint){
				.HObject = HObject,
				.Position = Position,
			},
	};
	arrput(GPhysics.Constraints, Constraint);
	uint32 RawHandle = (uint32)(arrlenu(GPhysics.Constraints) - 1);
	return (PhysicsConstraintHandle){RawHandle};
}

PhysicsConstraintHandle PhysicsAddLinkConstraint(
	PhysicsObjectHandle HObject0,
	PhysicsObjectHandle HObject1,
	real32 TargetDistance)
{
	PhysConstraint Constraint = (PhysConstraint){
		.Type = PhysConstraintType_Link,
		.Link =
			(PhysLinkConstraint){
				.HObject0 = HObject0,
				.HObject1 = HObject1,
				.TargetDistance = TargetDistance,
			},
	};
	arrput(GPhysics.Constraints, Constraint);
	uint32 RawHandle = (uint32)(arrlenu(GPhysics.Constraints) - 1);
	return (PhysicsConstraintHandle){RawHandle};
}

size_t PhysicsGetObjectCount(void)
{
	return arrlenu(GPhysics.Objects);
}

PhysicsObjectHandle PhysicsAddObject(const PhysicsObject* OptionalConfig)
{
	PhysicsObject Object = (OptionalConfig) ? *OptionalConfig : (PhysicsObject){0};
	Object.LastPosition = Object.Position;
	Object.Radius = MAX(Object.Radius, 0.1f);

	arrput(GPhysics.Objects, Object);
	uint32 RawHandle = (uint32)(arrlenu(GPhysics.Objects) - 1);
	return (PhysicsObjectHandle){RawHandle};
}

bool PhysObjectHandleIsValid(PhysicsObjectHandle Handle)
{
	return VALID_INDEX(Handle.Value, arrlenu(GPhysics.Objects));
}

PhysicsObject* PhysicsGetObject(PhysicsObjectHandle Handle)
{
	ASSERT(Handle.Value != KInvalidHandle);
	ASSERT(VALID_INDEX(Handle.Value, arrlenu(GPhysics.Objects)));
	return &GPhysics.Objects[Handle.Value];
}

PhysConstraint* PhysicsGetConstraint(PhysicsConstraintHandle Handle)
{
	ASSERT(Handle.Value != KInvalidHandle);
	ASSERT(VALID_INDEX(Handle.Value, arrlenu(GPhysics.Constraints)));
	return &GPhysics.Constraints[Handle.Value];
}

// Private Implementations
static void _PhysicsUpdateVertletObjects(float DeltaTime)
{
	for (ptrdiff_t ObjectIndex = 0; ObjectIndex < arrlen(GPhysics.Objects); ObjectIndex++) {
		VertletObjectUpdate(&GPhysics.Objects[ObjectIndex], DeltaTime);
	}
}

static void _PhysicsApplyConstraints(float DeltaTime)
{
	for (ptrdiff_t ConstraintIndex = 0; ConstraintIndex < arrlen(GPhysics.Constraints);
		 ConstraintIndex++)
	{
		PhysConstraint* Constraint = &GPhysics.Constraints[ConstraintIndex];
		switch (Constraint->Type) {
			case PhysConstraintType_Pin:
				if (PhysObjectHandleIsValid(Constraint->Pin.HObject)) {
					PhysicsGetObject(Constraint->Pin.HObject)->Position = Constraint->Pin.Position;
				}
				break;
			case PhysConstraintType_Link:
				{
					PhysicsObject* P0 = PhysicsGetObject(Constraint->Link.HObject0);
					PhysicsObject* P1 = PhysicsGetObject(Constraint->Link.HObject1);
					Vec2 Diff = Sub(P0->Position, P1->Position);
					real32 Dist = Len(Diff);
					Vec2 Dir = DivV2F(Diff, Dist);
					real32 Delta = (Constraint->Link.TargetDistance - Dist) * 0.5f;
					Vec2 DeltaV2 = Mul(Dir, Delta);
					P0->Position = Add(P0->Position, DeltaV2);
					P1->Position = Sub(P1->Position, DeltaV2);
				}
				break;
			default:
				break;
		}
	}

	float WorldWidth = 1920.0f, WorldHeight = 1080.0f;
	for (ptrdiff_t ObjectIndex = 0; ObjectIndex < arrlen(GPhysics.Objects); ObjectIndex++) {
		PhysicsObject* Object = &GPhysics.Objects[ObjectIndex];
		if (Object->Position.X > WorldWidth - Object->Radius)
			Object->Position.X = WorldWidth - Object->Radius;
		if (Object->Position.X < Object->Radius) Object->Position.X = Object->Radius;
		if (Object->Position.Y > WorldHeight - Object->Radius)
			Object->Position.Y = WorldHeight - Object->Radius;
		if (Object->Position.Y < Object->Radius) Object->Position.Y = Object->Radius;
		// Vec2 CenterToObject = Sub(Object->Position, KCenter);
		// real32 Distance = Len(CenterToObject);
		// if (Distance > KRadius - Object->Radius) {
		// 	Vec2 DirToObject = DivV2F(CenterToObject, Distance);
		// 	Object->Position = Add(KCenter, Mul(DirToObject, KRadius));
		// }
	}
}

static void _PhysicsApplyGravity(void)
{
	for (ptrdiff_t ObjectIndex = 0; ObjectIndex < arrlen(GPhysics.Objects); ObjectIndex++) {
		VertletObjectAccelerate(&GPhysics.Objects[ObjectIndex], KGravity);
	}
}

static void _PhysicsApplyHeat(void)
{
	for (ptrdiff_t ObjectIndex = 0; ObjectIndex < arrlen(GPhysics.Objects); ObjectIndex++) {
		VertletObjectAccelerate(
			&GPhysics.Objects[ObjectIndex], Mul(KHeatForce, GPhysics.Objects[ObjectIndex].Heat));
	}
}

static void _PhysicsSolveCollisions(void)
{
	ptrdiff_t Count = arrlen(GPhysics.Objects);
	for (ptrdiff_t Index0 = 0; Index0 < Count; Index0++) {
		PhysicsObject* Object0 = &GPhysics.Objects[Index0];
		for (ptrdiff_t Index1 = Index0 + 1; Index1 < Count; Index1++) {
			PhysicsObject* Object1 = &GPhysics.Objects[Index1];
			const Vec2 CollisionVec = Sub(Object0->Position, Object1->Position);
			real32 Distance = Len(CollisionVec);
			real32 ContactDistance = Object0->Radius + Object1->Radius;
			if (Distance < ContactDistance) {
				const Vec2 Direction = DivV2F(CollisionVec, Distance);
				Vec2 Delta = Mul(Direction, (ContactDistance - Distance) * 0.5f);

				Object0->Position = Add(Object0->Position, Delta);
				Object1->Position = Sub(Object1->Position, Delta);
				real32 HeatTransfer0 = Object0->Heat * 0.1f;
				real32 HeatTransfer1 = Object1->Heat * 0.1f;
				Object0->Heat -= HeatTransfer0;
				Object0->Heat += HeatTransfer1;
				Object1->Heat -= HeatTransfer1;
				Object1->Heat += HeatTransfer0;
			}
		}
	}
}
