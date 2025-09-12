#pragma once

#include "Math2D.h"

typedef struct SDL_Renderer SDL_Renderer;
typedef struct FluidSim FluidSim;

typedef struct FluidSimConfig {
	AABB Bounds;
} FluidSimConfig;

typedef struct LagrangianFluidSimConfig {
	FluidSimConfig Super;
} LagrangianFluidSimConfig;

// public interface
FluidSim *CreateLagrangianFluidSim(const LagrangianFluidSimConfig *Config);
void DestroyFluidSim(FluidSim *Self);
void FluidSimUpdate(FluidSim *Self, float32 DeltaTime);
void FluidSimRender(FluidSim *Self, SDL_Renderer *Renderer);
