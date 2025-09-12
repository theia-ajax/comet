#include "FluidSim.h"

#include <SDL3/SDL.h>
#include <stb_ds.h>

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
} FluidSim;

typedef struct EulerFluidSim {
	FluidSim Super;
	// TODO
} EulerFluidSim;

typedef struct FluidParticle {
	Vec2 Position;
	Vec2 Velocity;
	float32 Radius;
	float32 Mass;
} FluidParticle;

typedef struct LagrangianFluidSim {
	FluidSim Super;
	FluidParticle* Particles;
	float32 SmoothRadius;
	AABB Bounds;
} LagrangianFluidSim;

// private interface

FluidSim* AllocFluidSim(
	size_t FluidSimTypeSize,
	FluidSimOnCreate OnCreate,
	FluidSimOnDestroy OnDestroy,
	FluidSimOnUpdate OnUpdate,
	FluidSimOnRender OnRender);

float32 FluidSimSmoothingKernel(FluidSim* Self, float32 Radius, float32 Distance);
float32 CalculateDensity(FluidSim* Self, Vec2 SamplePoint);

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
	FLUID_SIM_ON_UPDATE(Self, DeltaTime);
}

void FluidSimRender(FluidSim* Self, SDL_Renderer* Renderer)
{
	FLUID_SIM_ON_RENDER(Self, Renderer);
}

float32 FluidSimSmoothingKernel(FluidSim* Self, float32 Radius, float32 Distance)
{
}

float32 CalculateDensity(FluidSim* Self, Vec2 SamplePoint)
{
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

float32 FluidSimSmoothingKernel(FluidSim* Self, float32 Radius, float32 Distance);
float32 CalculateDensity(FluidSim* Self, Vec2 SamplePoint);

void LagrangianFluidSimOnCreate(FluidSim* Sim, const FluidSimConfig* SimConfig)
{
	FLUID_SIM_SELF_AND_CONFIG(LagrangianFluidSim);

	ASSERT(Self != NULL);
	ASSERT(Config != NULL);

	Self->Bounds = Config->Super.Bounds;
	arrsetlen(Self->Particles, 128);

	for (int32 Index = 0, Count = arrlen(Self->Particles); Index < Count; Index++) {
		FluidParticle* Particle = &Self->Particles[Index];
		*Particle = (FluidParticle) {
			.Position = AABBRandomPosition(Self->Bounds),
			.Velocity = V2(0, 0),
			.Radius = 1.0f,
			.Mass = 1.0f,
		};
	}
}

void LagrangianFluidSimOnDestroy(FluidSim* Sim)
{
	FLUID_SIM_SELF(LagrangianFluidSim);
	arrfree(Self->Particles);
}

void LagrangianFluidSimOnUpdate(FluidSim* Sim, float32 DeltaTime)
{
	FLUID_SIM_SELF(LagrangianFluidSim);
}

void LagrangianFluidSimOnRender(FluidSim* Sim, SDL_Renderer* Renderer)
{
	FLUID_SIM_SELF(LagrangianFluidSim);
}