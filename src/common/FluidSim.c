#include "FluidSim.h"

#include <SDL3/SDL.h>
#include <stb_ds.h>

#include "Debug.h"
#include "Draw.h"
#include "Log.h"
#include "Random.h"

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

// private type definitions
typedef void (*FluidSimOnCreate)(FluidSim* Self, const FluidSimConfig* SimConfig);
typedef void (*FluidSimOnDestroy)(FluidSim* Self);
typedef void (*FluidSimOnUpdate)(FluidSim* Self, float32 DeltaTime);
typedef void (*FluidSimOnRender)(FluidSim* Self, SDL_Renderer* Renderer);

typedef struct FluidSim {
	struct {
		FluidSimOnCreate OnCreate;
		FluidSimOnDestroy OnDestroy;
		FluidSimOnUpdate OnUpdate;
		FluidSimOnRender OnRender;
	} VTable;
	float32 ElapsedTime;
} FluidSim;

typedef struct EulerFluidSim {
	FluidSim Super;
	// TODO
} EulerFluidSim;

typedef struct LagrangianFluidSim {
	FluidSim Super;
	AABB Bounds;
	float32 SmoothRadius;
	float32 ParticleRadius;
	float32 TargetDensity;
	float32 PressureMultiplier;
	float32 Gravity;
	struct {
		Vec2* Position;
		Vec2* Velocity;
		float32* Density;
	} P;
} LagrangianFluidSim;

// private interface

FluidSim* AllocFluidSim(
	size_t FluidSimTypeSize,
	FluidSimOnCreate OnCreate,
	FluidSimOnDestroy OnDestroy,
	FluidSimOnUpdate OnUpdate,
	FluidSimOnRender OnRender);

float32 _SmoothingKernel(float32 Radius, float32 Distance);
float32 _SmoothingKernelSlope(float32 Radius, float32 Distance);
float32 _CalculateDensity(LagrangianFluidSim* Self, Vec2 SamplePoint);
Vec2 _CalculatePressureForce(LagrangianFluidSim* Self, int32 ParticleIndex);
float32 _CalculateSharedPressure(LagrangianFluidSim* Self, float32 DensityA, float32 DensityB);
float32 _ConvertDensityToPressure(LagrangianFluidSim* Self, float32 Density);
float32 _ResolveCollisions(LagrangianFluidSim* Self, Vec2* Position, Vec2* Velocity);

void LagrangianFluidSimOnCreate(FluidSim* Sim, const FluidSimConfig* SimConfig);
void LagrangianFluidSimOnDestroy(FluidSim* Sim);
void LagrangianFluidSimOnUpdate(FluidSim* Sim, float32 DeltaTime);
void LagrangianFluidSimOnRender(FluidSim* Sim, SDL_Renderer* Renderer);

// Public implementations

FluidSim* CreateLagrangianFluidSim(const LagrangianFluidSimConfig* Config)
{
	FluidSim* Sim = AllocFluidSim(
		sizeof(LagrangianFluidSim),
		LagrangianFluidSimOnCreate,
		LagrangianFluidSimOnDestroy,
		LagrangianFluidSimOnUpdate,
		LagrangianFluidSimOnRender);

	FLUID_SIM_ON_CREATE(Sim, Config);

	return Sim;
}

void DestroyFluidSim(FluidSim* Self)
{
	FLUID_SIM_ON_DESTROY(Self);
	SDL_free(Self);
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

// Private implementations

FluidSim* AllocFluidSim(
	size_t FluidSimTypeSize,
	FluidSimOnCreate OnCreate,
	FluidSimOnDestroy OnDestroy,
	FluidSimOnUpdate OnUpdate,
	FluidSimOnRender OnRender)
{
	ASSERT(FluidSimTypeSize >= sizeof(FluidSim));

	FluidSim* Sim = SDL_malloc(FluidSimTypeSize);
	SDL_memset(Sim, 0, FluidSimTypeSize);

	Sim->VTable.OnCreate = OnCreate;
	Sim->VTable.OnDestroy = OnDestroy;
	Sim->VTable.OnUpdate = OnUpdate;
	Sim->VTable.OnRender = OnRender;

	return Sim;
}

void LagrangianFluidSimOnCreate(FluidSim* Sim, const FluidSimConfig* SimConfig)
{
	FLUID_SIM_SELF_AND_CONFIG(LagrangianFluidSim);

	ASSERT(Self != NULL);
	ASSERT(Config != NULL);

	Self->Bounds = Config->Super.Bounds;
	Self->SmoothRadius = 5;
	Self->ParticleRadius = 1;
	Self->TargetDensity = 2.75f;
	Self->PressureMultiplier = 4.0f;
	Self->Gravity = 0.0f;

	const int32 NumParticles = 5000;

	arrsetlen(Self->P.Position, NumParticles);
	arrsetlen(Self->P.Velocity, NumParticles); 
	arrsetlen(Self->P.Density, NumParticles);

	for (int32 Index = 0, Count = arrlen(Self->P.Position); Index < Count; Index++) {
		Self->P.Position[Index] = AABBRandomPosition(Self->Bounds);
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

void LagrangianFluidSimOnUpdate(FluidSim* Sim, float32 DeltaTime)
{
	FLUID_SIM_SELF(LagrangianFluidSim);

	float32 S = (SinF(Sim->ElapsedTime / 4) + 1) / 2.0f;
	// Self->SmoothRadius = Lerp(50.0f, 150.0f, S);

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

float32 _CalculateDensity(LagrangianFluidSim* Self, Vec2 SamplePoint)
{
	float32 Density = 0.0f;
	const float32 Mass = 1.0f;

	for (int32 Index = 0, Count = arrlen(Self->P.Position); Index < Count; Index++) {
		Vec2* Pos = &Self->P.Position[Index];
		float32 Distance = Len(Sub(*Pos, SamplePoint));
		float32 Influence = _SmoothingKernel(Self->SmoothRadius, Distance);
		Density += Mass * Influence;
	}

	return Density;
}

Vec2 _CalculatePressureForce(LagrangianFluidSim* Self, int32 ParticleIndex)
{
	Vec2 PressureForce = V2(0, 0);
	const float32 Mass = 1.0f;

	Vec2 SamplePoint = Self->P.Position[ParticleIndex];

	for (int32 Index = 0, Count = arrlen(Self->P.Position); Index < Count; Index++) {
		if (ParticleIndex == Index) {
			continue;
		}

		Vec2 Delta = Sub(Self->P.Position[Index], SamplePoint);
		float32 Distance = Len(Delta);
		Vec2 Direction = (Distance > 0) ? Div(Delta, Distance) : RandomDirection();
		float32 Slope = _SmoothingKernelSlope(Distance, Self->SmoothRadius);
		float32 Density = Self->P.Density[Index];
		float32 SharedPressure = -_CalculateSharedPressure(Self, Density, Self->P.Density[ParticleIndex]);
		PressureForce = Add(PressureForce, Mul(Direction, SharedPressure * Slope * Mass / Density));
	}

	return PressureForce;
}

float32 _CalculateSharedPressure(LagrangianFluidSim* Self, float32 DensityA, float32 DensityB)
{
	return (_ConvertDensityToPressure(Self, DensityA) + _ConvertDensityToPressure(Self, DensityB)) / 2;
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

void LagrangianFluidSimOnRender(FluidSim* Sim, SDL_Renderer* Renderer)
{
	FLUID_SIM_SELF(LagrangianFluidSim);

	const SDL_PixelFormatDetails* FormatDetails = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32);

	for (int32 Index = 0, Count = arrlen(Self->P.Position); Index < Count; Index++) {
		Vec2* Pos = &Self->P.Position[Index];

		DrawCircle(*Pos, Self->ParticleRadius, SDL_MapRGBA(FormatDetails, NULL, 0, 127, 255, 255));
	}
}
