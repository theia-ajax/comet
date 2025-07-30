#include "ParticleSandbox.h"

#include <SDL3/SDL.h>
#include <stb_ds.h>

#include "Log.h"
#include "Random.h"

typedef struct ParticleSpawner {
	Vec2 Position;
	Vec2 SpawnAcceleration;
	float32 SpawnInterval;
	float32 SpawnTimer;
	float32 ObjectRadius;
} ParticleSpawner;

typedef struct ParticleSandbox {
	ParticleSandboxConfig Config;
	ParticleSpawner* Spawners;
	bool SpawnersEnabled;
	PhysicsObject* ObjectBuffer;
} ParticleSandbox;

typedef struct HeatCompareUserData {
	bool Invert;
} HeatCompareUserData;

ParticleSandbox GSandbox;

void _ApplyConfigChanges(const ParticleSandboxConfig* Old, const ParticleSandboxConfig* New);
void _CreateSpawners(void);
void _DestroySpawners(void);
void _UpdateSpawners(float32 DeltaTime);
int _ParticleHeatCompareUserData(void* UserData, const void* A, const void* B);

void ParticleSandboxInitialize(const ParticleSandboxConfig* Config)
{
	ZERO_STRUCT(&GSandbox);

	GSandbox.Config = (Config) ? *Config : ParticleSandboxDefaultConfig();

	arrsetcap(GSandbox.ObjectBuffer, GSandbox.Config.Physics.MaxPhysicsObjects);

	PhysicsInitialize(&GSandbox.Config.Physics);
	_CreateSpawners();
	ParticleSandboxSetSpawnersEnabled(true);
}

void ParticleSandboxShutdown(void)
{
	arrfree(GSandbox.ObjectBuffer);
	_DestroySpawners();
	PhysicsShutdown();
}

ParticleSandboxConfig ParticleSandboxDefaultConfig(void)
{
	return (ParticleSandboxConfig){
		.Physics = PhysicsDefaultConfig(),
		.Spawners =
			{
				.Offset = (Vec2){16, 4},
				.Spacing = 4.0f,
				.Interval = 0.001f,
				.ObjectRadius = 1.0f,
			},
		.Rendering =
			{
				.Width = 360,
				.Height = 80,
				.RenderDriver = GetStringId("opengles2"),
				.HeatColorsImageFileName = GetStringId("assets/heat_color_ramp.generated.png"),
				.ParticleSpriteSheetName = GetStringId("SmallFlare"),
				.UseSpriteIndex = true,
				.ParticleSpriteIndex = 0,
				.SpriteScale = 0.01f,
				.ExtraRadius = 2.0f,
				.HeatScale = 32.0f,
				.InvertLayer = false,
				.UseHeatAsLayer = true,
			},
	};
}

const ParticleSandboxConfig ParticleSandboxGetConfig(void)
{
	return GSandbox.Config;
}

void ParticleSandboxSetSpawnersEnabled(bool Enabled)
{
	GSandbox.SpawnersEnabled = Enabled;
}

bool ParticleSandboxGetSpawnersEnabled(void)
{
	return GSandbox.SpawnersEnabled;
}

bool ParticleSandboxToggleSpawnersEnabled(void)
{
	GSandbox.SpawnersEnabled = !GSandbox.SpawnersEnabled;
	return GSandbox.SpawnersEnabled;
}

void ParticleSandboxReset(void)
{
	PhysicsClearAllObjects();
	ParticleSandboxSetSpawnersEnabled(true);
}

void ParticleSandboxRenderToTexture(ParticleSandboxRenderContext* Context)
{
	SDL_SetRenderTarget(Context->Renderer, Context->TargetTexture);

	SDL_SetRenderDrawColor(Context->Renderer, 0, 0, 0, 0);
	SDL_RenderClear(Context->Renderer);

	const PhysicsObject* Objects = PhysicsGetObjects();
	size_t ObjectCount = PhysicsGetObjectCount();

	arrsetlen(GSandbox.ObjectBuffer, ObjectCount);

	SDL_memcpy(GSandbox.ObjectBuffer, Objects, sizeof(*Objects) * ObjectCount);

	if (GSandbox.Config.Rendering.UseHeatAsLayer) {
		HeatCompareUserData UserData = {
			.Invert = GSandbox.Config.Rendering.InvertLayer,
		};

		SDL_qsort_r(
			GSandbox.ObjectBuffer,
			ObjectCount,
			sizeof(*GSandbox.ObjectBuffer),
			_ParticleHeatCompareUserData,
			&UserData);
	}

	SDL_Texture* Texture = Context->ParticleTexture;

	for (int32 ObjectIndex = 0; ObjectIndex < ObjectCount; ObjectIndex++) {
		PhysicsObject* Object = &GSandbox.ObjectBuffer[ObjectIndex];
		Vec2 Pos = Object->Position;
		float32 Radius = Object->Radius;
		SDL_FRect PosRect = {Pos.X - Radius, Pos.Y - Radius, Radius * 2 + 1, Radius * 2 + 1};
		ColorU8 TintColor;
		if ((Object->Flags & 1) != 0) {
			TintColor = Object->Tint;
		} else {
			ptrdiff_t len = arrlen(Context->HeatGradient);
			if (len > 0) {
				int32 Index = (int32)(MIN(Object->Heat, 1.0f - KEpsilonFloat32) * len);
				TintColor = Context->HeatGradient[Index];
			} else {
				TintColor = Object->Tint;
			}
		}

		float32 SpriteScale = GSandbox.Config.Rendering.SpriteScale;
		float32 HeatScale = GSandbox.Config.Rendering.HeatScale;
		float32 ExtraRadius = GSandbox.Config.Rendering.ExtraRadius;
		float32 Scale = (Radius + Object->Heat * HeatScale + ExtraRadius) * SpriteScale;
		float64 Rotation = 0.0;

		SDL_FRect SourceRect = {
			.x = 0.0f,
			.y = 0.0f,
			.w = (float32)Texture->w,
			.h = (float32)Texture->h,
		};

		float32 Width = SourceRect.w * Scale;
		float32 Height = SourceRect.h * Scale;

		SDL_FRect DestRect = {
			.x = Pos.X - Width / 2.0f,
			.y = Pos.Y - Height / 2.0f,
			.w = Width,
			.h = Height,
		};

		SDL_FPoint Center = {
			.x = DestRect.w / 2.0f,
			.y = DestRect.h / 2.0f,
		};

		SDL_SetTextureColorMod(Texture, TintColor.R, TintColor.G, TintColor.B);
		SDL_SetTextureAlphaMod(Texture, TintColor.A);

		SDL_RenderTextureRotated(Context->Renderer, Texture, &SourceRect, &DestRect, Rotation, &Center, SDL_FLIP_NONE);
	}
	SDL_SetRenderTarget(Context->Renderer, NULL);
}

void ParticleSandboxApplyConfig(const ParticleSandboxConfig* Config)
{
	_ApplyConfigChanges(&GSandbox.Config, Config);
}

void ParticleSandboxUpdate(float32 DeltaTime)
{
	_UpdateSpawners(DeltaTime);
	PhysicsUpdate(DeltaTime);
}

void _ApplyConfigChanges(const ParticleSandboxConfig* Old, const ParticleSandboxConfig* New)
{
}

void _CreateSpawners(void)
{
	arrsetlen(GSandbox.Spawners, 0);

	for (float32 SpawnerX = GSandbox.Config.Spawners.Offset.X + GSandbox.Config.Physics.Bounds.X;
		 SpawnerX < GSandbox.Config.Physics.Bounds.Z;
		 SpawnerX += Max(GSandbox.Config.Spawners.Spacing, 1.0f))
	{
		arrput(
			GSandbox.Spawners,
			((ParticleSpawner){
				.Position = V2(SpawnerX, GSandbox.Config.Spawners.Offset.Y),
				.SpawnAcceleration = V2(0, 0.0f),
				.SpawnInterval = GSandbox.Config.Spawners.Interval,
				.ObjectRadius = Max(GSandbox.Config.Spawners.ObjectRadius, 0.1f),
			}));
	}
}

void _DestroySpawners(void)
{
	arrfree(GSandbox.Spawners);
}

void _UpdateSpawners(float32 DeltaTime)
{
	if (GSandbox.SpawnersEnabled) {
		for (int32 SpawnerIndex = 0; SpawnerIndex < arrlen(GSandbox.Spawners); SpawnerIndex += 5) {
			ParticleSpawner* Spawner = &GSandbox.Spawners[SpawnerIndex];

			if (Spawner->SpawnTimer <= 0.0f) {
				Spawner->SpawnTimer += Spawner->SpawnInterval;
				Vec2 Accel = Spawner->SpawnAcceleration;
				Accel.X += RandomRangeF(-5000.0f, 5000.0f);
				PhysicsAddObject(&(PhysicsObject){
					.Position = Spawner->Position,
					.Radius = Spawner->ObjectRadius,
					.Acceleration = Accel,
					.Heat = 0.0f,
					.Flags = PhysicsObjectFlags_None,
					.Tint = (ColorU8){.R = 0xCC, .G = 0x00, .B = 0xCC, .A = 0xFF},
				});
			} else {
				Spawner->SpawnTimer -= DeltaTime;
			}
		}

		if (PhysicsGetObjectCount() >= PhysicsGetConfig()->MaxPhysicsObjects) {
			GSandbox.SpawnersEnabled = false;
		}
	}
}

inline int _FloatCompare(float32 A, float32 B)
{
	return (A < B) ? -1 : ((B > A) ? 1 : 0);
}

int _ParticleHeatCompareUserData(void* UserData, const void* A, const void* B)
{
	const HeatCompareUserData* Data = (const HeatCompareUserData*)UserData;
	const PhysicsObject* ObjA = (const PhysicsObject*)A;
	const PhysicsObject* ObjB = (const PhysicsObject*)B;

	return (!Data->Invert) ? _FloatCompare(ObjA->Heat, ObjB->Heat) : _FloatCompare(ObjB->Heat, ObjA->Heat);
}