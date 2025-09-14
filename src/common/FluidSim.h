#pragma once

#include "Math2D.h"

typedef struct SDL_Renderer SDL_Renderer;
typedef struct FluidSim FluidSim;
typedef struct LagrangianFluidSim LagrangianFluidSim;
struct json_value_s;
struct json_object_s;

typedef struct FluidSimConfig {
	AABB Bounds;
} FluidSimConfig;

typedef struct LagrangianFluidSimConfig {
	FluidSimConfig Super;
	int32 NumParticles;
	float32 SmoothRadius;
	float32 ParticleRadius;
	float32 ParticleMass;
	float32 TargetDensity;
	float32 PressureMultiplier;
	float32 Gravity;
} LagrangianFluidSimConfig;

// public interface
FluidSim* CreateLagrangianFluidSim(const LagrangianFluidSimConfig* Config);
void DestroyFluidSim(FluidSim* Self);
void FluidSimReconfig(FluidSim* Self, FluidSimConfig* Config);
void FluidSimUpdate(FluidSim* Self, float32 DeltaTime);
void FluidSimRender(FluidSim* Self, SDL_Renderer* Renderer);

void LagrangianFluidSimDebugRender(LagrangianFluidSim* Self, SDL_Renderer* Renderer, Vec2 MousePos);

bool JsonParseLagrangianFluidSimConfig(struct json_value_s* ConfigValue, LagrangianFluidSimConfig* ConfigOut);