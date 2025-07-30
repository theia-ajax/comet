#pragma once

#include "Math2D.h"
#include "ParticlePhysics.h"
#include "StringId.h"

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

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

typedef struct ParticleSandboxRenderContext {
	SDL_Renderer* Renderer;
	SDL_Texture* TargetTexture;
	SDL_Texture* ParticleTexture;
	ColorU8* HeatGradient;
} ParticleSandboxRenderContext;

void ParticleSandboxInitialize(const ParticleSandboxConfig* Config);
void ParticleSandboxShutdown(void);
ParticleSandboxConfig ParticleSandboxDefaultConfig(void);
const ParticleSandboxConfig ParticleSandboxGetConfig(void);

void ParticleSandboxSetSpawnersEnabled(bool Enabled);
bool ParticleSandboxGetSpawnersEnabled(void);
bool ParticleSandboxToggleSpawnersEnabled(void);

void ParticleSandboxReset(void);

void ParticleSandboxApplyConfig(const ParticleSandboxConfig* Config);

void ParticleSandboxUpdate(float32 DeltaTime);
void ParticleSandboxRenderToTexture(ParticleSandboxRenderContext* Context);