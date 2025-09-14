#include "FluidSim.h"

#include <SDL3/SDL.h>
#include <stb_ds.h>

#include "Algorithm.h"
#include "Debug.h"
#include "Draw.h"
#include "JsonHelpers.h"
#include "Log.h"
#include "Random.h"
#include "RenderUtil.h"

#define FLUID_SIM_DECLARE_TYPED_SELF(SimType, Sim) SimType* Self = SDL_static_cast(SimType*, Sim)
#define FLUID_SIM_DECLARE_TYPED_CONFIG(ConfigType, SimConfig)                                                          \
	const ConfigType* Config = SDL_static_cast(const ConfigType*, SimConfig)

#define FLUID_SIM_SELF(SimType) FLUID_SIM_DECLARE_TYPED_SELF(SimType, Sim);
#define FLUID_SIM_SELF_AND_CONFIG(SimType)                                                                             \
	FLUID_SIM_SELF(SimType);                                                                                           \
	FLUID_SIM_DECLARE_TYPED_CONFIG(CAT(SimType, Config), SimConfig);

#define FLUID_SIM_CALL(Sim, Func, ...)                                                                                 \
	if (Sim->VTable.Func != NULL) {                                                                                    \
		Sim->VTable.Func(Sim __VA_OPT__(, ) __VA_ARGS__);                                                              \
	}
#define FLUID_SIM_ON_CREATE(Sim, Config) FLUID_SIM_CALL(Sim, OnCreate, SDL_static_cast(FluidSimConfig*, Config))
#define FLUID_SIM_ON_DESTROY(Sim) FLUID_SIM_CALL(Sim, OnDestroy)
#define FLUID_SIM_ON_UPDATE(Sim, DeltaTime) FLUID_SIM_CALL(Sim, OnUpdate, DeltaTime)
#define FLUID_SIM_ON_RENDER(Sim, Renderer) FLUID_SIM_CALL(Sim, OnRender, Renderer)
#define FLUID_SIM_ON_RECONFIG(Sim, Config) FLUID_SIM_CALL(Sim, OnReconfig, SDL_static_cast(FluidSimConfig*, Config))

// private type definitions
typedef void (*FluidSimOnCreate)(FluidSim* Self, const FluidSimConfig* SimConfig);
typedef void (*FluidSimOnDestroy)(FluidSim* Self);
typedef void (*FluidSimOnUpdate)(FluidSim* Self, float32 DeltaTime);
typedef void (*FluidSimOnRender)(FluidSim* Self, SDL_Renderer* Renderer);
typedef void (*FluidSimOnReconfig)(FluidSim* Self, const FluidSimConfig* SimConfig);

typedef struct FluidSimVTable {
	FluidSimOnCreate OnCreate;
	FluidSimOnDestroy OnDestroy;
	FluidSimOnUpdate OnUpdate;
	FluidSimOnRender OnRender;
	FluidSimOnReconfig OnReconfig;
} FluidSimVTable;

typedef struct FluidSim {
	FluidSimVTable VTable;
	float32 ElapsedTime;
} FluidSim;

typedef struct EulerFluidSim {
	FluidSim Super;
	// TODO
} EulerFluidSim;

typedef struct SpatialLookupEntry {
	int32 ParticalIndex;
	uint32 CellId;
} SpatialLookupEntry;

typedef struct LagrangianFluidSim {
	FluidSim Super;
	AABB Bounds;
	float32 SmoothRadius;
	float32 ParticleRadius;
	float32 ParticleMass;
	float32 TargetDensity;
	float32 PressureMultiplier;
	float32 Gravity;
	struct {
		Vec2* Position;
		Vec2* Velocity;
		float32* Density;
		SpatialLookupEntry* SpatialLookup;
		uint32* StartIndices;
	} P;
} LagrangianFluidSim;

// static data

static const SDL_Point KCellOffsets[] = {
	{-1, -1},
	{0, -1},
	{1, -1},
	{-1, 0},
	{0, 0},
	{1, 0},
	{-1, 1},
	{0, 1},
	{1, 1},
};

// private interface

FluidSim* AllocFluidSim(size_t FluidSimTypeSize, const FluidSimVTable* VTable);

float32 _SmoothingKernel(float32 Radius, float32 Distance);
float32 _SmoothingKernelSlope(float32 Radius, float32 Distance);
float32 _CalculateDensity(LagrangianFluidSim* Self, Vec2 SamplePoint);
Vec2 _CalculatePressureForce(LagrangianFluidSim* Self, int32 ParticleIndex);
float32 _CalculateSharedPressure(LagrangianFluidSim* Self, float32 DensityA, float32 DensityB);
float32 _ConvertDensityToPressure(LagrangianFluidSim* Self, float32 Density);
float32 _ResolveCollisions(LagrangianFluidSim* Self, Vec2* Position, Vec2* Velocity);
void _PositionToCellCoord(Vec2 Pos, float32 Radius, int* XOut, int* YOut);
uint32 _HashCell(int X, int Y);
uint32 _GetCellIdFromHash(uint32 Hash, uint32 LookupSize);
void _UpdateSpatialLookup(LagrangianFluidSim* Self, Vec2* Points, float32 Radius);
AABB _AABBFromCellCord(int CellX, int CellY, float32 Radius);

typedef void (*LagrangianFluidSimParticleIndexCallback)(LagrangianFluidSim* Self, int32 Index, void* UserData);
void _ForEachParticleInRadius(
	LagrangianFluidSim* Self,
	Vec2 SamplePoint,
	float32 Radius,
	LagrangianFluidSimParticleIndexCallback Callback,
	void* UserData);
int32* _GetParticlesInRadius(LagrangianFluidSim* Self, Vec2 SamplePoint, float32 Radius);

void LagrangianFluidSimOnCreate(FluidSim* Sim, const FluidSimConfig* SimConfig);
void LagrangianFluidSimOnReconfig(FluidSim* Sim, const FluidSimConfig* SimConfig);
void LagrangianFluidSimOnDestroy(FluidSim* Sim);
void LagrangianFluidSimOnUpdate(FluidSim* Sim, float32 DeltaTime);
void LagrangianFluidSimOnRender(FluidSim* Sim, SDL_Renderer* Renderer);

// Public implementations

FluidSim* CreateLagrangianFluidSim(const LagrangianFluidSimConfig* Config)
{
	FluidSim* Sim = AllocFluidSim(
		sizeof(LagrangianFluidSim),
		&(FluidSimVTable){
			.OnCreate = LagrangianFluidSimOnCreate,
			.OnDestroy = LagrangianFluidSimOnDestroy,
			.OnUpdate = LagrangianFluidSimOnUpdate,
			.OnRender = LagrangianFluidSimOnRender,
			.OnReconfig = LagrangianFluidSimOnReconfig,
		});
	FLUID_SIM_ON_CREATE(Sim, Config);

	return Sim;
}

void DestroyFluidSim(FluidSim* Self)
{
	FLUID_SIM_ON_DESTROY(Self);
	SDL_free(Self);
}

void FluidSimReconfig(FluidSim* Self, FluidSimConfig* Config)
{
	FLUID_SIM_ON_RECONFIG(Self, Config);
}

void FluidSimUpdate(FluidSim* Self, float32 DeltaTime)
{
	Self->ElapsedTime += DeltaTime;

	FLUID_SIM_ON_UPDATE(Self, DeltaTime);
}

void FluidSimRender(FluidSim* Self, SDL_Renderer* Renderer)
{
	FLUID_SIM_ON_RENDER(Self, Renderer);
}

void LagrangianFluidSimDebugRender(LagrangianFluidSim* Self, SDL_Renderer* Renderer, Vec2 MousePos)
{
	const float32 Radius = Self->SmoothRadius;

	SDL_SetRenderDrawColor(Renderer, 0, 63, 255, 255);
	for (float32 Y = Self->Bounds.MinBound.Y; Y <= Self->Bounds.MaxBound.Y; Y += Radius) {
		SDL_RenderLine(Renderer, Self->Bounds.MinBound.X, Y, Self->Bounds.MaxBound.X, Y);
	}

	for (float32 X = Self->Bounds.MinBound.X; X <= Self->Bounds.MaxBound.X; X += Radius) {
		SDL_RenderLine(Renderer, X, Self->Bounds.MinBound.Y, X, Self->Bounds.MaxBound.Y);
	}

	SDL_SetRenderDrawColor(Renderer, 0, 255, 0, 255);
	SDL_RenderCoordinatesFromWindow(Renderer, MousePos.X, MousePos.Y, &MousePos.X, &MousePos.Y);
	SDL_RenderRect(Renderer, &(SDL_FRect){.x = MousePos.X - 2, .y = MousePos.Y - 2, .w = 4, .h = 4});

	SDL_SetRenderDrawColor(Renderer, 250, 50, 20, 255);
	int CenterX, CenterY;
	_PositionToCellCoord(MousePos, Radius, &CenterX, &CenterY);

	for (int OffsetIndex = 0; OffsetIndex < SDL_arraysize(KCellOffsets); OffsetIndex++) {
		int CellX = CenterX + KCellOffsets[OffsetIndex].x;
		int CellY = CenterY + KCellOffsets[OffsetIndex].y;
		SDL_RenderAABB(Renderer, _AABBFromCellCord(CellX, CellY, Radius));
	}

	int32* Particles = _GetParticlesInRadius(Self, MousePos, Radius);

	SDL_SetRenderDrawColor(Renderer, 200, 200, 0, 255);
	for (int32 Index = 0, Count = arrlen(Particles); Index < Count; Index++) {
		int32 ParticleIndex = Particles[Index];
		SDL_RenderAABB(Renderer, AABBFromCenterRadius(Self->P.Position[ParticleIndex], Self->ParticleRadius));
	}

	SDL_RenderCircle(Renderer, MousePos, Radius);

	arrfree(Particles);
}

bool JsonParseLagrangianFluidSimConfig(struct json_value_s* ConfigValue, LagrangianFluidSimConfig* ConfigOut)
{
	ASSERT(ConfigOut);
	ZERO_STRUCT(ConfigOut);

	struct json_object_s* ConfigObject = json_value_as_object(ConfigValue);

	if (ConfigObject == NULL) {
		return false;
	}

	ConfigOut->Super.Bounds = JsonGetAABB(ConfigObject, "bounds", (AABB){0, 0, 100, 100});
	ConfigOut->NumParticles = JsonGetInt32(ConfigObject, "num_particles", 0);
	ConfigOut->SmoothRadius = JsonGetFloat32(ConfigObject, "smooth_radius", 1.0f);
	ConfigOut->ParticleRadius = JsonGetFloat32(ConfigObject, "particle_radius", 1.0f);
	ConfigOut->ParticleMass = JsonGetFloat32(ConfigObject, "particle_mass", 1.0f);
	ConfigOut->TargetDensity = JsonGetFloat32(ConfigObject, "target_density", 1.0f);
	ConfigOut->PressureMultiplier = JsonGetFloat32(ConfigObject, "pressure_multiplier", 1.0f);
	ConfigOut->Gravity = JsonGetFloat32(ConfigObject, "gravity", 1.0f);

	return true;
}

// Private implementations

FluidSim* AllocFluidSim(size_t FluidSimTypeSize, const FluidSimVTable* VTable)
{
	ASSERT(FluidSimTypeSize >= sizeof(FluidSim));

	FluidSim* Sim = SDL_malloc(FluidSimTypeSize);
	SDL_memset(Sim, 0, FluidSimTypeSize);
	Sim->VTable = *VTable;
	return Sim;
}

void LagrangianFluidSimReconfig(LagrangianFluidSim* Self, const LagrangianFluidSimConfig* Config)
{
	Self->Bounds = Config->Super.Bounds;
	Self->SmoothRadius = Config->SmoothRadius;
	Self->ParticleRadius = Config->ParticleRadius;
	Self->ParticleMass = Config->ParticleMass;
	Self->TargetDensity = Config->TargetDensity;
	Self->PressureMultiplier = Config->PressureMultiplier;
	Self->Gravity = Config->Gravity;
}

void LagrangianFluidSimOnCreate(FluidSim* Sim, const FluidSimConfig* SimConfig)
{
	FLUID_SIM_SELF_AND_CONFIG(LagrangianFluidSim);

	ASSERT(Self != NULL);
	ASSERT(Config != NULL);

	LagrangianFluidSimReconfig(Self, Config);

	const int32 NumParticles = Config->NumParticles;

	arrsetlen(Self->P.Position, NumParticles);
	arrsetlen(Self->P.Velocity, NumParticles);
	arrsetlen(Self->P.Density, NumParticles);
	arrsetlen(Self->P.SpatialLookup, NumParticles);
	arrsetlen(Self->P.StartIndices, NumParticles);

	SDL_zeroarr(Self->P.Position);
	SDL_zeroarr(Self->P.Velocity);
	SDL_zeroarr(Self->P.Density);
	SDL_zeroarr(Self->P.SpatialLookup);
	SDL_zeroarr(Self->P.StartIndices);

	const float32 Spacing = 8.0f;
	Vec2 SpawnPos = Add(Self->Bounds.MinBound, V2(Spacing, Spacing));
	for (int32 Index = 0, Count = arrlen(Self->P.Position); Index < Count; Index++) {
		Self->P.Position[Index] = SpawnPos;
		SpawnPos.X += Spacing;
		if (SpawnPos.X > Self->Bounds.MaxBound.X - Spacing) {
			SpawnPos.X = Self->Bounds.MinBound.X + Spacing;
			SpawnPos.Y += Spacing;
		}

		Self->P.Position[Index] = RandomPositionInBounds(Self->Bounds, Self->ParticleRadius);
		// Self->P.Velocity[Index] = V2(RandomRangeF(-1.0f, 1.0f), RandomRangeF(-1.0f, 1.0f));
	}
}

void LagrangianFluidSimOnDestroy(FluidSim* Sim)
{
	FLUID_SIM_SELF(LagrangianFluidSim);
	arrfree(Self->P.Position);
	arrfree(Self->P.Velocity);
	arrfree(Self->P.Density);
}

void LagrangianFluidSimOnReconfig(FluidSim* Sim, const FluidSimConfig* SimConfig)
{
	FLUID_SIM_SELF_AND_CONFIG(LagrangianFluidSim);
	LagrangianFluidSimReconfig(Self, Config);
}

void LagrangianFluidSimOnUpdate(FluidSim* Sim, float32 DeltaTime)
{
	FLUID_SIM_SELF(LagrangianFluidSim);

	float32 S = (SinF(Sim->ElapsedTime / 4) + 1) / 2.0f;
	// Self->SmoothRadius = Lerp(50.0f, 150.0f, S);

	_UpdateSpatialLookup(Self, Self->P.Position, Self->SmoothRadius);

	for (int32 Index = 0, Count = arrlen(Self->P.Position); Index < Count; Index++) {
		Self->P.Velocity[Index] = Add(Self->P.Velocity[Index], Mul(V2(0, Self->Gravity), DeltaTime));
		Self->P.Density[Index] = _CalculateDensity(Self, Self->P.Position[Index]);
	}

	for (int32 Index = 0, Count = arrlen(Self->P.Position); Index < Count; Index++) {
		Vec2 PressureForce = _CalculatePressureForce(Self, Index);
		Vec2 PressureAcceleration = Div(PressureForce, Self->P.Density[Index]);
		Self->P.Velocity[Index] = Add(Self->P.Velocity[Index], Mul(PressureAcceleration, DeltaTime));
		// Self->P.Velocity[Index] = Mul(PressureAcceleration, DeltaTime);
	}

	// for (int32 Index = 0; Index < 1; Index++) {
	// 	DebugPrintf(
	// 		"Pos: %0.2f,%0.2f Vel: %0.2f,%0.2f Den: %f",
	// 		Self->P.Position[Index].X,
	// 		Self->P.Position[Index].Y,
	// 		Self->P.Velocity[Index].X,
	// 		Self->P.Velocity[Index].Y,
	// 		Self->P.Density[Index]);
	// }

	for (int32 Index = 0, Count = arrlen(Self->P.Position); Index < Count; Index++) {
		Vec2* Pos = &Self->P.Position[Index];
		Vec2* Vel = &Self->P.Velocity[Index];
		*Pos = Add(*Pos, *Vel);
		_ResolveCollisions(Self, Pos, Vel);
	}
}

float32 _SmoothingKernel(float32 Radius, float32 Distance)
{
	if (Distance >= Radius) {
		return 0.0f;
	}

	float32 Volume = SDL_PI_F * pow(Radius, 4) / 6;
	return SQUARE(Radius - Distance) / Volume;
}

float32 _SmoothingKernelSlope(float32 Radius, float32 Distance)
{
	if (Distance >= Radius) {
		return 0.0f;
	}
	float32 Scale = 12 / (pow(Radius, 4) * SDL_PI_F);
	return (Distance - Radius) * Scale;
}

typedef struct CalcDensityCallbackContext {
	Vec2 SamplePoint;
	float32 Radius;
	float32 Density;
} CalcDensityCallbackContext;

void _CalcDensityCallback(LagrangianFluidSim* Self, int32 ParticleIndex, void* UserData)
{
	CalcDensityCallbackContext* Context = (CalcDensityCallbackContext*)UserData;

	Vec2* Pos = &Self->P.Position[ParticleIndex];
	float32 Distance = Len(Sub(*Pos, Context->SamplePoint));
	float32 Influence = _SmoothingKernel(Context->Radius, Distance);
	Context->Density += Self->ParticleMass * Influence;
}

float32 _CalculateDensity(LagrangianFluidSim* Self, Vec2 SamplePoint)
{
	CalcDensityCallbackContext Context = (CalcDensityCallbackContext){
		.SamplePoint = SamplePoint,
		.Radius = Self->SmoothRadius,
		.Density = 0.0f,
	};

	_ForEachParticleInRadius(Self, SamplePoint, Self->SmoothRadius, _CalcDensityCallback, &Context);

	return Context.Density;
}

typedef struct CalcPressureForceCallbackContext {
	Vec2 SamplePoint;
	float32 Radius;
	int32 ParticleIndex;
	Vec2 PressureForce;
} CalcPressureForceCallbackContext;

void _CalculatePressureForceCallback(LagrangianFluidSim* Self, int32 OtherParticleIndex, void* UserData)
{
	CalcPressureForceCallbackContext* Context = (CalcPressureForceCallbackContext*)UserData;

	if (Context->ParticleIndex != OtherParticleIndex) {
		Vec2 Delta = Sub(Self->P.Position[OtherParticleIndex], Context->SamplePoint);
		float32 Distance = Len(Delta);
		Vec2 Direction = (Distance > 0) ? Div(Delta, Distance) : RandomDirection();
		float32 Slope = _SmoothingKernelSlope(Context->Radius, Distance);
		float32 Density = Self->P.Density[OtherParticleIndex];
		float32 SharedPressure = -_CalculateSharedPressure(Self, Density, Self->P.Density[Context->ParticleIndex]);
		float32 ForceScalar = SharedPressure * Slope * Self->ParticleMass / Density;
		Context->PressureForce = Add(Context->PressureForce, Mul(Direction, ForceScalar));
	}
}

Vec2 _CalculatePressureForce(LagrangianFluidSim* Self, int32 ParticleIndex)
{
	CalcPressureForceCallbackContext Context = (CalcPressureForceCallbackContext){
		.SamplePoint = Self->P.Position[ParticleIndex],
		.Radius = Self->SmoothRadius,
		.ParticleIndex = ParticleIndex,
		.PressureForce = V2(0, 0),
	};

	_ForEachParticleInRadius(Self, Context.SamplePoint, Context.Radius, _CalculatePressureForceCallback, &Context);

	return Context.PressureForce;
}

float32 _CalculateSharedPressure(LagrangianFluidSim* Self, float32 DensityA, float32 DensityB)
{
	float32 PressureA = _ConvertDensityToPressure(Self, DensityA);
	float32 PressureB = _ConvertDensityToPressure(Self, DensityB);
	return (PressureA + PressureB) / 2.0f;
}

float32 _ConvertDensityToPressure(LagrangianFluidSim* Self, float32 Density)
{
	float32 DensityError = Density - Self->TargetDensity;
	float32 Pressure = DensityError * Self->PressureMultiplier;
	return Pressure;
}

float32 _ResolveCollisions(LagrangianFluidSim* Self, Vec2* Position, Vec2* Velocity)
{
	float32 Radius = Self->ParticleRadius;
	float32 Left = Self->Bounds.MinBound.X + Radius;
	float32 Right = Self->Bounds.MaxBound.X - Radius;
	float32 Top = Self->Bounds.MinBound.Y + Radius;
	float32 Bottom = Self->Bounds.MaxBound.Y - Radius;

	const float32 Elasticity = 0.85f;

	if (Position->X < Left || Position->X > Right) {
		Velocity->X = -Velocity->X * Elasticity;
		Position->X = Clamp(Position->X, Left, Right);
	}

	if (Position->Y < Top || Position->Y > Bottom) {
		Velocity->Y = -Velocity->Y * Elasticity;
		Position->Y = Clamp(Position->Y, Top, Bottom);
	}
}

void _PositionToCellCoord(Vec2 Pos, float32 Radius, int* XOut, int* YOut)
{
	*XOut = (int)(Pos.X / Radius);
	*YOut = (int)(Pos.Y / Radius);
}

AABB _AABBFromCellCord(int CellX, int CellY, float32 Radius)
{
	AABB Result = (AABB){
		.MinBound = V2(CellX * Radius, CellY * Radius),
	};
	Result.MaxBound = Add(Result.MinBound, V2(Radius, Radius));
	return Result;
}

uint32 _HashCell(int X, int Y)
{
	return (uint32)X * 15823 + (uint32)Y * 9737333;
}

uint32 _GetCellIdFromHash(uint32 Hash, uint32 LookupSize)
{
	return Hash % LookupSize;
}

uint32 _GetCellId(int X, int Y, uint32 NumBuckets)
{
	return ((uint32)X * 15823 + (uint32)Y * 9737333) % NumBuckets;
}

uint32 _GetCellIdFromPosition(Vec2 Position, float32 Radius, uint32 LookupSize)
{
	int X, Y;
	_PositionToCellCoord(Position, Radius, &X, &Y);
	return _GetCellId(X, Y, LookupSize);
}

int SDLCALL SpatialLookupCompareCallback(const void* A, const void* B)
{
	SpatialLookupEntry* EntryA = (SpatialLookupEntry*)A;
	SpatialLookupEntry* EntryB = (SpatialLookupEntry*)B;
	return EntryA->CellId < EntryB->CellId ? -1 : (EntryA->CellId > EntryB->CellId ? 1 : 0);
}

void _UpdateSpatialLookup(LagrangianFluidSim* Self, Vec2* Points, float32 Radius)
{
	for (int32 Index = 0, Count = arrlen(Points); Index < Count; Index++) {
		uint32 CellId = _GetCellIdFromPosition(Points[Index], Radius, arrlenu(Self->P.SpatialLookup));
		Self->P.SpatialLookup[Index] = (SpatialLookupEntry){
			.ParticalIndex = Index,
			.CellId = CellId,
		};
		Self->P.StartIndices[Index] = UINT32_MAX;
	}

	SDL_qsort(
		Self->P.SpatialLookup,
		arrlenu(Self->P.SpatialLookup),
		sizeof(Self->P.SpatialLookup[0]),
		SpatialLookupCompareCallback);

	for (int32 Index = 0, Count = arrlen(Points); Index < Count; Index++) {
		uint32 Key = Self->P.SpatialLookup[Index].CellId;
		uint32 KeyPrev = Index == 0 ? UINT32_MAX : Self->P.SpatialLookup[Index - 1].CellId;
		if (Key != KeyPrev) {
			Self->P.StartIndices[Key] = Index;
		}
	}
}

void _ForEachParticleInRadius(
	LagrangianFluidSim* Self,
	Vec2 SamplePoint,
	float32 Radius,
	LagrangianFluidSimParticleIndexCallback Callback,
	void* UserData)
{
	int CenterX, CenterY;
	_PositionToCellCoord(SamplePoint, Radius, &CenterX, &CenterY);
	for (int OffsetIndex = 0; OffsetIndex < SDL_arraysize(KCellOffsets); OffsetIndex++) {
		int CellX = CenterX + KCellOffsets[OffsetIndex].x;
		int CellY = CenterY + KCellOffsets[OffsetIndex].y;
		uint32 CellId = _GetCellId(CellX, CellY, arrlenu(Self->P.SpatialLookup));
		uint32 CellStartIndex = Self->P.StartIndices[CellId];
		for (uint32 Index = CellStartIndex, Count = arrlenu(Self->P.SpatialLookup); Index < Count; Index++) {
			if (Self->P.SpatialLookup[Index].CellId != CellId) {
				break;
			}

			int32 ParticleIndex = Self->P.SpatialLookup[Index].ParticalIndex;
			float32 SqrDistance = LenSqr(Sub(Self->P.Position[ParticleIndex], SamplePoint));
			if (SqrDistance <= SQUARE(Radius)) {
				Callback(Self, ParticleIndex, UserData);
			}
		}
	}
}

void _AddParticleToListCallback(LagrangianFluidSim* Self, int32 Index, void* UserData)
{
	int32* ParticleIndices = (int32*)UserData;
	arrput(ParticleIndices, Index);
}

int32* _GetParticlesInRadius(LagrangianFluidSim* Self, Vec2 SamplePoint, float32 Radius)
{
	int32* ParticlesIndices = NULL;
	arrsetcap(ParticlesIndices, 256);

	_ForEachParticleInRadius(Self, SamplePoint, Radius, _AddParticleToListCallback, (void*)ParticlesIndices);

	return ParticlesIndices;
}

void LagrangianFluidSimOnRender(FluidSim* Sim, SDL_Renderer* Renderer)
{
	FLUID_SIM_SELF(LagrangianFluidSim);

	const SDL_PixelFormatDetails* FormatDetails = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32);

	for (int32 Index = 0, Count = arrlen(Self->P.Position); Index < Count; Index++) {
		Vec2* Pos = &Self->P.Position[Index];

		DrawCircle(*Pos, Self->ParticleRadius, SDL_MapRGBA(FormatDetails, NULL, 0, 127, 255, 255));
	}

	// DrawAABB(Self->Bounds, 0xFFFFFF00);
}
