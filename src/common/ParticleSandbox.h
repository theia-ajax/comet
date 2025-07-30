#pragma once

#include "Math2D.h"
#include "ParticlePhysics.h"
#include "StringId.h"

typedef struct ParticleSandboxRenderingConfig {
	int32 Width;
	int32 Height;
	StringId RenderDriver;
	StringId HeatColorsImageFileName;
	float32 SpriteScale;
	float32 ExtraRadius;
	float32 HeatScale;
	StringId ParticleSpriteName;
	StringId ParticleSpriteSheetName;
	int32 ParticleSpriteIndex;
	bool UseSpriteIndex;
	bool UseHeatAsLayer;
	bool InvertLayer;
} ParticleSandboxRenderingConfig;

typedef struct ParticleSandboxSpawnersConfig {
	Vec2 Offset;
	float32 Spacing;
	float32 Interval;
	float32 ObjectRadius;

} ParticleSandboxSpawnersConfig;

typedef struct ParticleSandboxConfig {
	ParticleSandboxRenderingConfig Rendering;
	ParticleSandboxSpawnersConfig Spawners;
	PhysicsConfig Physics;
} ParticleSandboxConfig;

typedef struct ParticlePhysicsConfigFile {
	ParticleSandboxConfig Config;
	struct {
		const char* FileName;
		SDL_Time LastModified;
	} Meta;
} ParticlePhysicsConfigFile;

void ParticleSandboxInitialize(void);