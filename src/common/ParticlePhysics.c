#include "ParticlePhysics.h"

#include <SDL2/SDL_thread.h>
#include <stb_ds.h>

#define USE_GRID_SOLVER

// Constants
static const uint32 KInvalidHandle = (uint32)-1;

const PhysicsConfig KDefaultPhysicsConfig = (PhysicsConfig){
	.Bounds = (Vec4){0, 0, 1920, 1080},
	.CellSize = 24.0f,
	.Gravity = (Vec2){0.0f, 100.0f},
	.HeatForce = (Vec2){0.0f, -200.0f},
};

enum { KMaxObjectsPerCell = 64 };

// Private Definitions
typedef struct PhysCell {
	FixedList(PhysicsObjectHandle, KMaxObjectsPerCell) Objects;
} PhysCell;

struct {
	PhysicsConfig Config;
	PhysicsObject* Objects;
	PhysConstraint* Constraints;
	PhysCell* Grid;
	int32 GridWidth;
	int32 GridHeight;
} GPhysics;

// Private Prototypes
static void _PhysicsObjectUpdate(PhysicsObject* Object, float DeltaTime);
static void _PhysicsUpdateVertletObjects(float DeltaTime);
static void _PhysicsApplyAllConstraints(float DeltaTime);
static void _PhysicsApplyGravity(void);
static void _PhysicsApplyHeat(void);
static void _PhysicsSolveCellCollisions(PhysCell* Cell0, PhysCell* Cell1);
static void _PhysicsSolveAllCollisions(void);
static void _PhysicsSolveCollision(PhysicsObject* Object0, PhysicsObject* Object1);
static void _PhysicsUpdateGridObjectHandles(void);
static int32 _PhysicsGridIndexXY(int32 GridX, int32 GridY);
static int32 _PhysicsGetWorldPositionGridIndex(Vec2 WorldPosition);
static PhysCell* _PhysicsGetCellWorldPosition(Vec2 Position);
static PhysCell* _PhysicsGetCellGridXY(int32 GridX, int32 GridY);
// static void _PhysicsApplyConstraint(PhysConstraint* Constraint);

static inline Point _GridIndexToPoint(int32 GridIndex)
{
	return (Point){GridIndex % GPhysics.GridWidth, GridIndex / GPhysics.GridWidth};
}

// Public Implementations
void PhysicsObjectAccelerate(PhysicsObject* Object, Vec2 Acceleration)
{
	Object->Acceleration = Add(Object->Acceleration, Acceleration);
}

void PhysicsInitialize(const PhysicsConfig* Config)
{
	GPhysics.Config = (Config != NULL) ? *Config : KDefaultPhysicsConfig;

	Vec2 WorldMin = GPhysics.Config.Bounds.XY;
	Vec2 WorldMax = GPhysics.Config.Bounds.ZW;

	real32 WorldWidth = WorldMax.X - WorldMin.X;
	real32 WorldHeight = WorldMax.Y - WorldMin.Y;
	GPhysics.GridWidth = (int32)ceil(WorldWidth / GPhysics.Config.CellSize);
	GPhysics.GridHeight = (int32)ceil(WorldHeight / GPhysics.Config.CellSize);
	arrsetlen(GPhysics.Grid, GPhysics.GridWidth * GPhysics.GridHeight);

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
		_PhysicsApplyAllConstraints(DeltaTime);
		_PhysicsUpdateVertletObjects(SubDeltaTime);
		_PhysicsUpdateGridObjectHandles();
		_PhysicsSolveAllCollisions();
	}
}

PhysicsConfig PhysicsDefaultConfig(void)
{
	return KDefaultPhysicsConfig;
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
	Object.LastGridCell = NONE;
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

PhysicsObject* PhysicsGetPinConstraintObject(PhysicsConstraintHandle Handle)
{
	PhysConstraint* Constraint = PhysicsGetConstraint(Handle);
	ASSERT(Constraint != NULL);
	ASSERT(Constraint->Type == PhysConstraintType_Pin);

	PhysicsObject* Object = NULL;
	if (VALID_INDEX(Handle.Value, arrlenu(GPhysics.Objects))) {
		Object = PhysicsGetObject(Constraint->Pin.HObject);
	}
	return Object;
}

bool PhysicsIsAreaClear(Vec2 Position)
{
	bool Result = true;
	int32 GridIndex = _PhysicsGetWorldPositionGridIndex(Position);
	Point CellPoint = _GridIndexToPoint(GridIndex);
	for (int32 Y = CellPoint.Y - 1; Y <= CellPoint.Y + 1 && Result; Y++) {
		for (int32 X = CellPoint.X - 1; X <= CellPoint.X + 1 && Result; X++) {
			PhysCell* Cell = _PhysicsGetCellGridXY(X, Y);
			if (Cell->Objects.Count > 0) {
				Result = false;
				break;
			}
		}
	}
	return Result;
}

#include <SDL2/SDL_render.h>
void PhysicsDebugDraw(SDL_Renderer* Renderer)
{
	SDL_SetRenderDrawColor(Renderer, 0, 0xCC, 0, 255);

	Vec2 BoundMin = GPhysics.Config.Bounds.XY;
	Vec2 BoundMax = GPhysics.Config.Bounds.ZW;

	SDL_FRect Rect = {
		.x = BoundMin.X,
		.y = BoundMin.Y,
		.w = BoundMax.X - BoundMin.X,
		.h = BoundMax.Y - BoundMin.Y,
	};
	for (int32 CellY = 0; CellY <= GPhysics.GridHeight; CellY++) {
		SDL_RenderDrawLineF(
			Renderer,
			BoundMin.X,
			CellY * GPhysics.Config.CellSize + BoundMin.Y,
			BoundMax.X,
			CellY * GPhysics.Config.CellSize + BoundMin.Y);
	}

	for (int32 CellX = 0; CellX <= GPhysics.GridWidth; CellX++) {
		SDL_RenderDrawLineF(
			Renderer,
			CellX * GPhysics.Config.CellSize + BoundMin.X,
			BoundMin.Y,
			CellX * GPhysics.Config.CellSize + BoundMin.X,
			BoundMax.Y);
	}

	for (int32 GridIndex = 0; GridIndex < arrlen(GPhysics.Grid); GridIndex++) {
		if (GPhysics.Grid[GridIndex].Objects.Count > 0) {
			SDL_SetRenderDrawColor(Renderer, 0, 0xCC, 0xCC, 255);
			Vec2 GridPos =
				Mul(V2(GridIndex % GPhysics.GridWidth, GridIndex / GPhysics.GridWidth), GPhysics.Config.CellSize);
			SDL_FRect GridRect = {
				.x = GridPos.X + BoundMin.X,
				.y = GridPos.Y + BoundMin.Y,
				.w = GPhysics.Config.CellSize,
				.h = GPhysics.Config.CellSize,
			};
			SDL_RenderFillRectF(Renderer, &GridRect);
		}
	}

	// SDL_RenderDrawRectF(Renderer, &Rect);
}

// Private Implementations
static void _PhysicsObjectUpdate(PhysicsObject* Object, float DeltaTime)
{
	Object->SecondsAlive += DeltaTime;

	Vec2 Velocity = Sub(Object->Position, Object->LastPosition);
	Object->LastPosition = Object->Position;
	Object->Position = Add(Add(Object->Position, Velocity), Mul(Object->Acceleration, DeltaTime * DeltaTime));
	Object->Heat -= (Object->Heat * 0.4f) * DeltaTime;

	real32 HeaterDistance = 20.0f;
	real32 HeaterThreshold = GPhysics.Config.Bounds.W - HeaterDistance;

	if (Object->Position.Y > HeaterThreshold) {
		Object->Heat += 2.0f * DeltaTime;
	} else if (Object->Position.Y < 24) {
		// Object->Heat -= 2.0f * DeltaTime;
	}
	Object->Heat = Clamp(Object->Heat, 0, 1);
	ZERO_STRUCT(&Object->Acceleration);

	// Object->GridCell = _PhysicsGetWorldPositionGridIndex(Object->Position);
}

static void _PhysicsUpdateVertletObjects(float DeltaTime)
{
	for (ptrdiff_t ObjectIndex = 0; ObjectIndex < arrlen(GPhysics.Objects); ObjectIndex++) {
		_PhysicsObjectUpdate(&GPhysics.Objects[ObjectIndex], DeltaTime);
	}
}

static void _PhysicsApplyAllConstraints(float DeltaTime)
{
	for (ptrdiff_t ConstraintIndex = 0; ConstraintIndex < arrlen(GPhysics.Constraints); ConstraintIndex++) {
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

	Vec2 CellDim = V2(GPhysics.Config.CellSize, GPhysics.Config.CellSize);
	Vec4 Bounds = GPhysics.Config.Bounds;
	Vec2 WorldMin = Add(GPhysics.Config.Bounds.XY, CellDim);
	Vec2 WorldMax = Sub(GPhysics.Config.Bounds.ZW, CellDim);
	for (ptrdiff_t ObjectIndex = 0; ObjectIndex < arrlen(GPhysics.Objects); ObjectIndex++) {
		PhysicsObject* Object = &GPhysics.Objects[ObjectIndex];

		if (Object->Position.X > WorldMax.X - Object->Radius) Object->Position.X = WorldMax.X - Object->Radius;
		if (Object->Position.X < WorldMin.X + Object->Radius) Object->Position.X = WorldMin.X + Object->Radius;
		if (Object->Position.Y > WorldMax.Y - Object->Radius) Object->Position.Y = WorldMax.Y - Object->Radius;
		if (Object->Position.Y < WorldMin.Y + Object->Radius) Object->Position.Y = WorldMin.Y + Object->Radius;
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
		PhysicsObjectAccelerate(&GPhysics.Objects[ObjectIndex], GPhysics.Config.Gravity);
	}
}

static void _PhysicsApplyHeat(void)
{
	for (ptrdiff_t ObjectIndex = 0; ObjectIndex < arrlen(GPhysics.Objects); ObjectIndex++) {
		PhysicsObjectAccelerate(
			&GPhysics.Objects[ObjectIndex], Mul(GPhysics.Config.HeatForce, GPhysics.Objects[ObjectIndex].Heat));
	}
}

static void _PhysicsSolveCellCollisions(PhysCell* Cell0, PhysCell* Cell1)
{
	if (Cell0 == NULL || Cell1 == NULL) {
		return;
	}

	for (int32 SubIndex0 = 0; SubIndex0 < Cell0->Objects.Count; SubIndex0++) {
		PhysicsObject* Object0 = PhysicsGetObject(*FixedListAt(Cell0->Objects, SubIndex0));
		for (int32 SubIndex1 = 0; SubIndex1 < Cell1->Objects.Count; SubIndex1++) {
			PhysicsObject* Object1 = PhysicsGetObject(*FixedListAt(Cell1->Objects, SubIndex1));

			if (Object0 != Object1) {
				_PhysicsSolveCollision(Object0, Object1);
			}
		}
	}
}

typedef struct _ChunkSolverCtx {
	int32 StartX;
	int32 EndX;
} _ChunkSolverCtx;

int _PhysicsSolveChunkWorker(void* Data)
{
	_ChunkSolverCtx* Context = (_ChunkSolverCtx*)Data;
	for (int32 CellY = 0; CellY < GPhysics.GridHeight; CellY++) {
		for (int32 CellX = Context->StartX; CellX < Context->EndX; CellX++) {
			PhysCell* Cell0 = _PhysicsGetCellGridXY(CellX, CellY);
			for (int32 LocalX = -1; LocalX <= 1; LocalX++) {
				for (int32 LocalY = -1; LocalY <= 1; LocalY++) {
					PhysCell* Cell1 = _PhysicsGetCellGridXY(CellX + LocalX, CellY + LocalY);
					_PhysicsSolveCellCollisions(Cell0, Cell1);
				}
			}
		}
	}
}

static void _PhysicsSolveAllCollisions(void)
{
#ifdef USE_GRID_SOLVER
	enum { KChunks = 2 };
	_Static_assert(KChunks > 0);

	if (KChunks == 1) {
		for (int32 CellY = 0; CellY < GPhysics.GridHeight; CellY++) {
			for (int32 CellX = 0; CellX < GPhysics.GridWidth; CellX++) {
				PhysCell* Cell0 = _PhysicsGetCellGridXY(CellX, CellY);
				for (int32 LocalX = -1; LocalX <= 1; LocalX++) {
					for (int32 LocalY = -1; LocalY <= 1; LocalY++) {
						PhysCell* Cell1 = _PhysicsGetCellGridXY(CellX + LocalX, CellY + LocalY);
						_PhysicsSolveCellCollisions(Cell0, Cell1);
					}
				}
			}
		}
	} else {
		_ChunkSolverCtx ContextStorage[KChunks] = {0};
		SDL_Thread* Threads[KChunks] = {NULL};

		int32 ColsPerChunk = GPhysics.GridWidth / KChunks;
		for (int32 ChunkIndex = 0; ChunkIndex < KChunks; ChunkIndex++) {
			_ChunkSolverCtx* Ctx = &ContextStorage[ChunkIndex];
			Ctx->StartX = ChunkIndex * ColsPerChunk;
			if (ChunkIndex < KChunks - 1) {
				Ctx->EndX = (ChunkIndex + 1) * ColsPerChunk;
			} else {
				Ctx->EndX = GPhysics.GridWidth;
			}

			Threads[ChunkIndex] = SDL_CreateThread(_PhysicsSolveChunkWorker, "ChunkWorker", Ctx);
		}

		for (int32 ThreadIndex = 0; ThreadIndex < KChunks; ThreadIndex++) {
			SDL_WaitThread(Threads[ThreadIndex], NULL);
		}
	}
#else
	ptrdiff_t Count = arrlen(GPhysics.Objects);
	for (ptrdiff_t Index0 = 0; Index0 < Count; Index0++) {
		PhysicsObject* Object0 = &GPhysics.Objects[Index0];
		for (ptrdiff_t Index1 = Index0 + 1; Index1 < Count; Index1++) {
			PhysicsObject* Object1 = &GPhysics.Objects[Index1];
			_PhysicsSolveCollision(Object0, Object1);
		}
	}
#endif
}

static void _PhysicsSolveCollision(PhysicsObject* Object0, PhysicsObject* Object1)
{
	const Vec2 CollisionVec = Sub(Object0->Position, Object1->Position);
	real32 Distance = Len(CollisionVec);
	real32 ContactDistance = Object0->Radius + Object1->Radius;
	if (Distance < ContactDistance) {
		const Vec2 Direction = DivV2F(CollisionVec, Distance);
		Vec2 Delta = Mul(Direction, (ContactDistance - Distance) * 0.5f);

		Object0->Position = Add(Object0->Position, Delta);
		Object1->Position = Sub(Object1->Position, Delta);

		if (Object0->Heat > Object1->Heat) {
			real32 Transfer = Object0->Heat * 0.05f;
			Object0->Heat -= Transfer;
			Object1->Heat += Transfer;
		} else if (Object0->Heat < Object1->Heat) {
			real32 Transfer = Object1->Heat * 0.05f;
			Object0->Heat += Transfer;
			Object1->Heat -= Transfer;
		}
	}
}

static void _PhysicsUpdateGridObjectHandles(void)
{
	for (int32 ObjectIndex = 0; ObjectIndex < arrlen(GPhysics.Objects); ObjectIndex++) {
		PhysicsObject* Object = &GPhysics.Objects[ObjectIndex];

		if (Object->LastGridCell != NONE) {
			for (int32 GridObjectIndex = 0; GridObjectIndex < GPhysics.Grid[Object->LastGridCell].Objects.Count;
				 GridObjectIndex++)
			{
				if (GPhysics.Grid[Object->LastGridCell].Objects.Data[GridObjectIndex].Value == ObjectIndex) {
					FixedListRemoveAt(GPhysics.Grid[Object->LastGridCell].Objects, GridObjectIndex);
					break;
				}
			}
		}

		Object->LastGridCell = Object->GridCell;

		PhysCell* Cell = _PhysicsGetCellWorldPosition(GPhysics.Objects[ObjectIndex].Position);

		if (Cell != NULL) {
			ASSERT(Cell->Objects.Count < FixedListCapacity(Cell->Objects));
			*FixedListPush(Cell->Objects) = (PhysicsObjectHandle){ObjectIndex};
			GPhysics.Objects[ObjectIndex].GridCell = Cell - GPhysics.Grid;
		} else {
			GPhysics.Objects[ObjectIndex].GridCell = NONE;
		}
	}
}

static int32 _PhysicsGridIndexXY(int32 GridX, int32 GridY)
{
	int32 Result = NONE;
	if (GridX >= 0 && GridX < GPhysics.GridWidth && GridY >= 0 && GridY < GPhysics.GridHeight) {
		Result = GridY * GPhysics.GridWidth + GridX;
	}
	return Result;
}

static int32 _PhysicsGetWorldPositionGridIndex(Vec2 WorldPosition)
{
	int32 Result = NONE;

	Vec2 WorldMin = GPhysics.Config.Bounds.XY;
	Vec2 WorldMax = GPhysics.Config.Bounds.ZW;
	Vec2 PhysPosition = Sub(WorldPosition, WorldMin);
	Vec2 GridPosition = DivV2F(PhysPosition, GPhysics.Config.CellSize);
	Result = _PhysicsGridIndexXY(GridPosition.X, GridPosition.Y);

	return Result;
}

static PhysCell* _PhysicsGetCellWorldPosition(Vec2 WorldPosition)
{
	PhysCell* Result = NULL;
	int32 GridIndex = _PhysicsGetWorldPositionGridIndex(WorldPosition);
	if (VALID_INDEX(GridIndex, arrlen(GPhysics.Grid))) {
		Result = &GPhysics.Grid[GridIndex];
	}
	return Result;
}

static PhysCell* _PhysicsGetCellGridXY(int32 GridX, int32 GridY)
{
	PhysCell* Result = NULL;
	int32 Index = _PhysicsGridIndexXY(GridX, GridY);
	if (VALID_INDEX(Index, arrlen(GPhysics.Grid))) {
		Result = &GPhysics.Grid[Index];
	}
	return Result;
}
