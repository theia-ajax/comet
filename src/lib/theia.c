#include <stdio.h>

#include "Log.h"
#include "ParticlePhysics.h"

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

void Initialize(void);
void Shutdown(void);

void StepSimulation(float32 DeltaTime);

void Initialize(void)
{
	LoggingInitialize(LogLevel_Info);
	
	PhysicsConfig Config = PhysicsDefaultConfig();
	PhysicsInitialize(&Config);
}

void Shutdown(void)
{
	PhysicsShutdown();
	LoggingShutdown();
}

void StepSimulation(float32 DeltaTime)
{
	
}

