#include "Physics.h"

#include <stb_ds.h>

// Constants
const Vec2 KGravity = (Vec2){0.0f, 1000.0f};
const Vec2 KHeatForce = (Vec2){0.0f, -4000.0f};

// Private Definitions
struct {
	VertletObject* Objects;
} GPhysics;

// Private Prototypes
static void _PhysicsUpdateVertletObjects(float DeltaTime);
static void _PhysicsApplyConstraint(float DeltaTime);
static void _PhysicsApplyGravity(void);
static void _PhysicsApplyHeat(void);
static void _PhysicsSolveCollisions(void);

// Public Implementations
void VertletObjectUpdate(VertletObject* Object, float DeltaTime)
{
	Vec2 Velocity = Sub(Object->Position, Object->LastPosition);
	Object->LastPosition = Object->Position;
	Object->Position =
		Add(Add(Object->Position, Velocity), Mul(Object->Acceleration, DeltaTime * DeltaTime));
	Object->Heat -= 0.1f * DeltaTime;
	if (Object->Position.Y > 300) {
		Object->Heat += 10.0f * DeltaTime;
	} else if (Object->Position.Y < 24) {
		Object->Heat -= 2.0f * DeltaTime;
	}
	Object->Heat = Clamp(Object->Heat, 0, 1);
	ZERO_STRUCT(&Object->Acceleration);
}

void VertletObjectAccelerate(VertletObject* Object, Vec2 Acceleration)
{
	Object->Acceleration = Add(Object->Acceleration, Acceleration);
}

void PhysicsInitialize(void)
{
	arrsetcap(GPhysics.Objects, 1024);
}

void PhysicsShutdown(void)
{
	arrfree(GPhysics.Objects);
}

void PhysicsUpdate(float DeltaTime)
{
	enum { KSubSteps = 8 };
	real32 SubDeltaTime = DeltaTime / KSubSteps;

	for (int32 SubStepIndex = 0; SubStepIndex < KSubSteps; SubStepIndex++) {
		_PhysicsApplyGravity();
		_PhysicsApplyHeat();
		_PhysicsApplyConstraint(DeltaTime);
		_PhysicsSolveCollisions();
		_PhysicsUpdateVertletObjects(SubDeltaTime);
	}
}

const VertletObject* PhysicsGetObjects(void)
{
	return GPhysics.Objects;
}

size_t PhysicsGetObjectCount(void)
{
	return arrlenu(GPhysics.Objects);
}

VertletObject* PhysicsAddObject(const VertletObject* OptionalConfig)
{
	VertletObject Object = (OptionalConfig) ? *OptionalConfig : (VertletObject){0};
	Object.LastPosition = Object.Position;
	Object.Radius = MAX(Object.Radius, 0.1f);

	arrput(GPhysics.Objects, Object);
	return arrlastp(GPhysics.Objects);
}

// Private Implementations
static void _PhysicsUpdateVertletObjects(float DeltaTime)
{
	for (ptrdiff_t ObjectIndex = 0; ObjectIndex < arrlen(GPhysics.Objects); ObjectIndex++) {
		VertletObjectUpdate(&GPhysics.Objects[ObjectIndex], DeltaTime);
	}
}

static void _PhysicsApplyConstraint(float DeltaTime)
{
	const Vec2 KCenter = V2(288, 162);
	const real32 KRadius = 150.0f;

	for (ptrdiff_t ObjectIndex = 0; ObjectIndex < arrlen(GPhysics.Objects); ObjectIndex++) {
		VertletObject* Object = &GPhysics.Objects[ObjectIndex];
		if (Object->Position.X > 576 - Object->Radius) Object->Position.X = 576 - Object->Radius;
		if (Object->Position.X < Object->Radius) Object->Position.X = Object->Radius;
		if (Object->Position.Y > 324 - Object->Radius) Object->Position.Y = 324 - Object->Radius;
		if (Object->Position.Y < Object->Radius) Object->Position.Y = Object->Radius;
		Object->Heat = Clamp(Object->Heat, 0.0f, 1.0f);
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
		VertletObject* Object0 = &GPhysics.Objects[Index0];
		for (ptrdiff_t Index1 = Index0 + 1; Index1 < Count; Index1++) {
			VertletObject* Object1 = &GPhysics.Objects[Index1];
			const Vec2 CollisionVec = Sub(Object0->Position, Object1->Position);
			real32 Distance = Len(CollisionVec);
			real32 ContactDistance = Object0->Radius + Object1->Radius;
			if (Distance < ContactDistance) {
				const Vec2 Direction = DivV2F(CollisionVec, Distance);
				Vec2 Delta = Mul(Direction, (ContactDistance - Distance) * 0.5f);

				real32 HeatDelta = ABS(Object0->Heat - Object1->Heat) * 0.02f;
				if (Object0->Heat > Object1->Heat) {
					Object0->Heat -= HeatDelta;
					Object1->Heat += HeatDelta;
				} else {
					Object1->Heat -= HeatDelta;
					Object0->Heat += HeatDelta;
				}
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
