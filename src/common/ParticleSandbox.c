#include "ParticleSandbox.h"

#include <SDL3/SDL.h>
#include <stb_ds.h>
#include <stdio.h>

#include "ColorUtil.h"
#include "Debug.h"
#include "FileUtil.h"
#include "IniHelpers.h"
#include "Log.h"
#include "Random.h"
#include "SdlEventHandler.h"
#include "SpriteDatabase.h"

typedef struct ParticleSpawner {
	Vec2 Position;
	Vec2 SpawnAcceleration;
	float32 SpawnInterval;
	float32 SpawnTimer;
	float32 ObjectRadius;
} ParticleSpawner;

typedef struct ParticleSandbox {
	ParticlePhysicsConfigFile ParticlePhysicsConfigFile;
	ParticleSandboxConfig* Config;
	ColorU8* HeatGradient;
	SpriteSheetId ParticleSpriteSheetId;
	SpriteId ParticleSpriteId;
	SDL_Texture* ParticleTexture;
	SDL_Texture* TargetTexture;
	ParticleSpawner* Spawners;
	bool SpawnersEnabled;
	PhysicsObject* ObjectBuffer;
} ParticleSandbox;

ParticleSandbox GSandbox;

void _CreateSpawners(void);
void _DestroySpawners(void);
void _UpdateSpawners(float32 DeltaTime);
int _ParticleHeatCompare(const void* A, const void* B);
int _ParticleHeatReverseCompare(const void* A, const void* B);
bool _ReadConfigFile(const char* FileName, ParticlePhysicsConfigFile* ConfigOut);
void _LoadHeatGradient(const char* GradientFileName);
void _UpdateParticleSpriteId(void);
bool _CheckConfigFileChanges(void);
void _ApplyConfigFileChanges(const ParticlePhysicsConfigFile* Old, const ParticlePhysicsConfigFile* New);
SDL_Texture* _GetTargetTexture(SDL_Renderer* Renderer);

bool ParticleSandboxHandleSdlEvent(const SDL_Event* Event, void* Context);

void _ParticleSandboxInitializeCommon(const ParticleSandboxConfig* Config)
{
	GSandbox.ParticlePhysicsConfigFile.Config = (Config) ? *Config : ParticleSandboxDefaultConfig();
	GSandbox.Config = &GSandbox.ParticlePhysicsConfigFile.Config;

	AddSdlEventHandler(ParticleSandboxHandleSdlEvent, NULL);

	arrsetcap(GSandbox.ObjectBuffer, GSandbox.Config->Physics.MaxPhysicsObjects);

	PhysicsInitialize(&GSandbox.Config->Physics);
	_CreateSpawners();
	ParticleSandboxSetSpawnersEnabled(true);
}

void ParticleSandboxInitialize(const ParticleSandboxConfig* Config)
{
	ZERO_STRUCT(&GSandbox);
	_ParticleSandboxInitializeCommon(Config);
}

void ParticleSandboxInitializeFromConfigFile(const char* ConfigFileName)
{
	GSandbox.ParticlePhysicsConfigFile.Config.Physics = PhysicsDefaultConfig();
	_ReadConfigFile("particles.ini", &GSandbox.ParticlePhysicsConfigFile);
	_ParticleSandboxInitializeCommon(&GSandbox.ParticlePhysicsConfigFile.Config);

	_LoadHeatGradient(StringIdCStr(GSandbox.Config->Rendering.HeatColorsImageFileName));
	_UpdateParticleSpriteId();

	{
		const uint32 HeatRampSize = 1024;
		arrsetlen(GSandbox.HeatGradient, HeatRampSize);
		GradientColorPoint ColorPoints[] = {
			(GradientColorPoint){.Position = 0, .Color = V4(0, 0, 0, 1)},
			(GradientColorPoint){.Position = 0.25f, .Color = V4(151 / 255.0f, 42 / 255.0f, 68 / 255.0f, 1)},
			(GradientColorPoint){.Position = 0.6f, .Color = V4(236 / 255.0f, 49 / 255.0f, 216 / 255.0f, 1)},
			(GradientColorPoint){.Position = 1, .Color = V4(1, 1, 1, 1)},
		};
		ColorGradientFromColorPoints(ColorPoints, ARRAY_COUNT(ColorPoints), HeatRampSize, GSandbox.HeatGradient);
	}
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
	return GSandbox.ParticlePhysicsConfigFile.Config;
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
	ASSERT(Context != NULL);
	ASSERT(Context->Renderer != NULL);

	Context->HeatGradient = (Context->HeatGradient) ? Context->HeatGradient : GSandbox.HeatGradient;
	Context->ParticleTexture =
		(Context->ParticleTexture) ? Context->ParticleTexture : GetSpriteTexture(GSandbox.ParticleSpriteId);
	Context->TargetTexture = (Context->TargetTexture) ? Context->TargetTexture : _GetTargetTexture(Context->Renderer);

	const PhysicsObject* Objects = PhysicsGetObjects();
	size_t ObjectCount = PhysicsGetObjectCount();

	arrsetlen(GSandbox.ObjectBuffer, ObjectCount);

	SDL_memcpy(GSandbox.ObjectBuffer, Objects, sizeof(*Objects) * ObjectCount);

	if (GSandbox.Config->Rendering.UseHeatAsLayer) {
		SDL_qsort(
			GSandbox.ObjectBuffer,
			ObjectCount,
			sizeof(*GSandbox.ObjectBuffer),
			GSandbox.Config->Rendering.InvertLayer ? _ParticleHeatReverseCompare : _ParticleHeatCompare);
	}

	SDL_Texture* Texture = Context->ParticleTexture;
	SDL_SetRenderTarget(Context->Renderer, Context->TargetTexture);
	SDL_SetRenderDrawColor(Context->Renderer, 0, 0, 0, 0);
	SDL_RenderClear(Context->Renderer);

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

		float32 SpriteScale = GSandbox.Config->Rendering.SpriteScale;
		float32 HeatScale = GSandbox.Config->Rendering.HeatScale;
		float32 ExtraRadius = GSandbox.Config->Rendering.ExtraRadius;
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

void ParticleSandboxRender(SDL_Renderer* Renderer)
{
	SDL_Texture* TargetTexture = _GetTargetTexture(Renderer);
	ParticleSandboxRenderToTexture(&(ParticleSandboxRenderContext){
		.Renderer = Renderer,
		.TargetTexture = TargetTexture,
		.ParticleTexture = GSandbox.ParticleTexture,
		.HeatGradient = GSandbox.HeatGradient,
	});
	SDL_RenderTexture(Renderer, TargetTexture, NULL, NULL);
}

void ParticleSandboxUpdate(float32 DeltaTime)
{
	_UpdateSpawners(DeltaTime);
	PhysicsUpdate(DeltaTime);

	DebugPrintf("Physics Objects: %d/%d", arrlen(GSandbox.ObjectBuffer), GSandbox.Config->Physics.MaxPhysicsObjects);
}

bool ParticleSandboxExportHeatGradientToFile(const char* FileName)
{
	ColorGradientExportToImageFile(
		"assets/heat_color_ramp.generated.png",
		GSandbox.HeatGradient,
		arrlenu(GSandbox.HeatGradient));
}

void ParticleSandboxDebugDraw(SDL_Renderer* Renderer)
{
	int WindowWidth, WindowHeight;
	SDL_GetCurrentRenderOutputSize(Renderer, &WindowWidth, &WindowHeight);

	for (uint32 x = 0, ColorCount = arrlenu(GSandbox.HeatGradient); x < ColorCount; x++) {
		ColorU8 C = GSandbox.HeatGradient[x];
		SDL_SetRenderDrawColor(Renderer, C.R, C.G, C.B, C.A);
		SDL_RenderRect(Renderer, &(SDL_FRect){.x = WindowWidth - ColorCount + x, .y = 0, .w = 1.0f, .h = 4.0f});
	}
}

void _CreateSpawners(void)
{
	arrsetlen(GSandbox.Spawners, 0);

	for (float32 SpawnerX = GSandbox.Config->Spawners.Offset.X + GSandbox.Config->Physics.Bounds.X;
		 SpawnerX < GSandbox.Config->Physics.Bounds.Z;
		 SpawnerX += Max(GSandbox.Config->Spawners.Spacing, 1.0f))
	{
		arrput(
			GSandbox.Spawners,
			((ParticleSpawner){
				.Position = V2(SpawnerX, GSandbox.Config->Spawners.Offset.Y),
				.SpawnAcceleration = V2(0, 0.0f),
				.SpawnInterval = GSandbox.Config->Spawners.Interval,
				.ObjectRadius = Max(GSandbox.Config->Spawners.ObjectRadius, 0.1f),
			}));
	}
}

void _DestroySpawners(void)
{
	arrfree(GSandbox.Spawners);
}

void _UpdateSpawners(float32 DeltaTime)
{
	_CheckConfigFileChanges();

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

int _ParticleHeatCompare(const void* A, const void* B)
{
	const PhysicsObject* ObjA = (const PhysicsObject*)A;
	const PhysicsObject* ObjB = (const PhysicsObject*)B;
	return COMPARE(ObjA->Heat, ObjB->Heat);
}

int _ParticleHeatReverseCompare(const void* A, const void* B)
{
	return _ParticleHeatCompare(B, A);
}

bool _ReadConfigFile(const char* FileName, ParticlePhysicsConfigFile* ConfigOut)
{
	char* FileData;
	if (ReadFileToNewBuffer(FileName, &FileData)) {
		ini_t* Ini = ini_load(FileData, NULL);
		SDL_free(FileData);

		if (!Ini) {
			return false;
		}

		ConfigOut->Meta.FileName = FileName;
		ParticleSandboxConfig* Sandbox = &ConfigOut->Config;

		if (ConfigOut->Meta.LastModified == 0) {
			SDL_PathInfo PathInfo;
			SDL_Time LastModifiedTime = 0;
			if (SDL_GetPathInfo(FileName, &PathInfo)) {
				LastModifiedTime = PathInfo.modify_time;
			}
			ConfigOut->Meta.LastModified = LastModifiedTime;
		}

		{
			int Section = ini_find_section(Ini, "Rendering", 0);

			Sandbox->Rendering.Width = IniReadInt(Ini, Section, "Width", 360);
			Sandbox->Rendering.Height = IniReadInt(Ini, Section, "Height", 80);
			Sandbox->Rendering.HeatColorsImageFileName =
				IniReadStringId(Ini, Section, "HeatColorsImage", KInvalidStringId);
			Sandbox->Rendering.SpriteScale = IniReadFloat(Ini, Section, "SpriteScale", 1.0);
			Sandbox->Rendering.HeatScale = IniReadFloat(Ini, Section, "HeatScale", 0.0);
			Sandbox->Rendering.ExtraRadius = IniReadFloat(Ini, Section, "ExtraRadius", 0.0);
			Sandbox->Rendering.ParticleSpriteName =
				IniReadStringId(Ini, Section, "ParticleSprite", GetStringId("explosion-01"));
			Sandbox->Rendering.ParticleSpriteSheetName =
				IniReadStringId(Ini, Section, "ParticleSpriteSheet", KInvalidStringId);
			Sandbox->Rendering.ParticleSpriteIndex = IniReadInt(Ini, Section, "ParticleSpriteIndex", 0);
			Sandbox->Rendering.UseSpriteIndex = IniReadBool(Ini, Section, "UseSpriteIndex", false);
			Sandbox->Rendering.UseHeatAsLayer = IniReadBool(Ini, Section, "UseHeatAsLayer", true);
			Sandbox->Rendering.InvertLayer = IniReadBool(Ini, Section, "InvertLayer", false);
		}

		{
			int Section = ini_find_section(Ini, "Spawners", 0);

			Sandbox->Spawners.Offset.X = IniReadFloat(Ini, Section, "OffsetX", 0.0);
			Sandbox->Spawners.Offset.Y = IniReadFloat(Ini, Section, "OffsetY", 0.0);
			Sandbox->Spawners.Interval = IniReadFloat(Ini, Section, "Interval", 0.0);
			Sandbox->Spawners.Spacing = IniReadFloat(Ini, Section, "Spacing", 32.0);
			Sandbox->Spawners.ObjectRadius = IniReadFloat(Ini, Section, "ObjectRadius", 1.0);
		}

		{
			int Section = ini_find_section(Ini, "Physics", 0);
			Sandbox->Physics.MaxPhysicsObjects = IniReadInt(Ini, Section, "MaxObjectCount", 128);

			Sandbox->Physics.Bounds.X = IniReadFloat(Ini, Section, "BoundsOffsetX", 0.0);
			Sandbox->Physics.Bounds.Y = IniReadFloat(Ini, Section, "BoundsOffsetY", 0.0);

			float32 BoundsWidth = IniReadFloat(Ini, Section, "BoundsWidth", 320);
			float32 BoundsHeight = IniReadFloat(Ini, Section, "BoundsHeight", 80);
			if (BoundsWidth < KEpsilonFloat32) {
				BoundsWidth = 320;
			};
			if (BoundsWidth < KEpsilonFloat32) {
				BoundsHeight = 80;
			};

			Sandbox->Physics.Bounds.Z = Sandbox->Physics.Bounds.X + BoundsWidth;
			Sandbox->Physics.Bounds.W = Sandbox->Physics.Bounds.Y + BoundsHeight;

			Sandbox->Physics.CellSize = IniReadFloat(Ini, Section, "CellSize", 8.0);

			Sandbox->Physics.Gravity.X = IniReadFloat(Ini, Section, "GravityX", 0.0);
			Sandbox->Physics.Gravity.Y = IniReadFloat(Ini, Section, "GravityY", 100.0);

			Sandbox->Physics.HeatForce.X = IniReadFloat(Ini, Section, "HeatForceX", 0.0);
			Sandbox->Physics.HeatForce.Y = IniReadFloat(Ini, Section, "HeatForceY", -160.0);

			Sandbox->Physics.HeatTransferRate = IniReadFloat(Ini, Section, "HeatTransferRate", 0.0);

			Sandbox->Physics.HeatDecay = IniReadFloat(Ini, Section, "HeatDecay", 0.0);
			Sandbox->Physics.HeaterZoneSize = IniReadFloat(Ini, Section, "HeaterZoneSize", 0.0);
			Sandbox->Physics.HeaterHeatDelta = IniReadFloat(Ini, Section, "HeaterHeatDelta", 0.0);
			Sandbox->Physics.CoolerZoneSize = IniReadFloat(Ini, Section, "CoolerZoneSize", 0.0);
			Sandbox->Physics.CoolerHeatDelta = IniReadFloat(Ini, Section, "CoolerHeatDelta", 0.0);

			Sandbox->Physics.SquishZoneSize = IniReadFloat(Ini, Section, "SquishZoneSize", 0.0);
			Sandbox->Physics.SquishZoneForceMin = IniReadFloat(Ini, Section, "SquishZoneForceMin", 0.0);
			Sandbox->Physics.SquishZoneForceMax =
				IniReadFloat(Ini, Section, "SquishZoneForceMax", Sandbox->Physics.SquishZoneForceMin);

			Sandbox->Physics.SurfaceTensionScalar = IniReadFloat(Ini, Section, "SurfaceTension", 0.0);
			Sandbox->Physics.SurfaceTensionExtraRadius = IniReadFloat(Ini, Section, "SurfaceTensionExtraRadius", 0.0);
		}

		ini_destroy(Ini);
	}

	return true;
}

bool _CheckConfigFileChanges(void)
{
	SDL_PathInfo PathInfo;
	SDL_Time LastModifiedTime = 0;
	if (SDL_GetPathInfo(GSandbox.ParticlePhysicsConfigFile.Meta.FileName, &PathInfo)) {
		LastModifiedTime = PathInfo.modify_time;
	}

	if (LastModifiedTime > GSandbox.ParticlePhysicsConfigFile.Meta.LastModified) {
		ParticlePhysicsConfigFile OldConfig = GSandbox.ParticlePhysicsConfigFile;

		_ReadConfigFile(GSandbox.ParticlePhysicsConfigFile.Meta.FileName, &GSandbox.ParticlePhysicsConfigFile);
		GSandbox.ParticlePhysicsConfigFile.Meta.LastModified = LastModifiedTime;

		_ApplyConfigFileChanges(&OldConfig, &GSandbox.ParticlePhysicsConfigFile);
	}
}

void _ApplyConfigFileChanges(const ParticlePhysicsConfigFile* Old, const ParticlePhysicsConfigFile* New)
{
	if (SDL_memcmp(&Old->Config.Physics, &New->Config.Physics, sizeof(Old->Config.Physics)) != 0) {
		if (Old->Config.Physics.MaxPhysicsObjects != New->Config.Physics.MaxPhysicsObjects) {
			if (New->Config.Physics.MaxPhysicsObjects < Old->Config.Physics.MaxPhysicsObjects) {
				PhysicsClearAllObjects();
			}
			ParticleSandboxSetSpawnersEnabled(true);
		}

		PhysicsReconfigure(&New->Config.Physics);
	}

	if (!StringIdEq(Old->Config.Rendering.HeatColorsImageFileName, New->Config.Rendering.HeatColorsImageFileName)) {

		_LoadHeatGradient(StringIdCStr(New->Config.Rendering.HeatColorsImageFileName));
	}

	if (!StringIdEq(Old->Config.Rendering.ParticleSpriteName, New->Config.Rendering.ParticleSpriteName) ||
		!StringIdEq(Old->Config.Rendering.ParticleSpriteSheetName, New->Config.Rendering.ParticleSpriteSheetName) ||
		Old->Config.Rendering.ParticleSpriteIndex != New->Config.Rendering.ParticleSpriteIndex)
	{
		_UpdateParticleSpriteId();
	}

	if (SDL_memcmp(&Old->Config.Physics.Bounds, &New->Config.Physics.Bounds, sizeof(Old->Config.Physics.Bounds)) != 0 ||
		SDL_memcmp(&Old->Config.Spawners, &New->Config.Spawners, sizeof(Old->Config.Spawners)) != 0)
	{
		// CreateSpawners();
	}
}

void _UpdateParticleSpriteId(void)
{
	if (StringIdIsValid(GSandbox.Config->Rendering.ParticleSpriteSheetName)) {
		GSandbox.ParticleSpriteSheetId = SpriteSheetFindByName(GSandbox.Config->Rendering.ParticleSpriteSheetName);

		if (GSandbox.Config->Rendering.UseSpriteIndex) {
			GSandbox.ParticleSpriteId = SpriteSheetFindSpriteByIndex(
				GSandbox.ParticleSpriteSheetId,
				GSandbox.Config->Rendering.ParticleSpriteIndex);
		} else {
			GSandbox.ParticleSpriteId = SpriteSheetFindSpriteByNameId(
				GSandbox.ParticleSpriteSheetId,
				GSandbox.Config->Rendering.ParticleSpriteName);
		}
	} else {
		GSandbox.ParticleSpriteId = SpriteFindByNameId(GSandbox.Config->Rendering.ParticleSpriteName);
	}

	GSandbox.ParticleTexture = GetSpriteTexture(GSandbox.ParticleSpriteId);
}

void _LoadHeatGradient(const char* GradientFileName)
{
	ImageAsset* HeatRampImage = (ImageAsset*)LoadAsset(AssetType_Image, GradientFileName);

	if (HeatRampImage) {
		arrsetcap(GSandbox.HeatGradient, HeatRampImage->Data->Width);
		arrsetlen(GSandbox.HeatGradient, 0);

		const SDL_PixelFormatDetails* FormatDetails = SDL_GetPixelFormatDetails(HeatRampImage->Data->Surface->format);

		{
			uint8* Begin = HeatRampImage->Data->Pixels;
			uint8* End =
				HeatRampImage->Data->Pixels + (HeatRampImage->Data->Width * HeatRampImage->Data->BytesPerPixel);
			for (uint8* Pixel = Begin; Pixel != End; Pixel += HeatRampImage->Data->BytesPerPixel) {
				ColorU8 PixelColor;
				SDL_GetRGBA(
					*((uint32*)Pixel),
					FormatDetails,
					NULL,
					&PixelColor.R,
					&PixelColor.G,
					&PixelColor.B,
					&PixelColor.A);
				arrput(GSandbox.HeatGradient, PixelColor);
			}
		}
	}
}

SDL_Texture* _GetTargetTexture(SDL_Renderer* Renderer)
{
	if (GSandbox.TargetTexture == NULL) {
		SDL_PropertiesID RendererProperties = SDL_GetRendererProperties(Renderer);
		SDL_PixelFormat* RendererPixelFormats =
			SDL_GetPointerProperty(SDL_GetRendererProperties(Renderer), SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER, NULL);

		SDL_PixelFormat PixelFormat = SDL_PIXELFORMAT_UNKNOWN;
		if (RendererPixelFormats) {
			PixelFormat = RendererPixelFormats[0];
		}

		GSandbox.TargetTexture = SDL_CreateTexture(
			Renderer,
			PixelFormat,
			SDL_TEXTUREACCESS_TARGET,
			GSandbox.Config->Rendering.Width,
			GSandbox.Config->Rendering.Height);
	}
	return GSandbox.TargetTexture;
}

bool ParticleSandboxHandleSdlEvent(const SDL_Event* Event, void* Context)
{
	bool Handled = false;
	switch (Event->type) {
		case SDL_EVENT_KEY_DOWN:
			switch (Event->key.scancode) {
				case SDL_SCANCODE_F4:
					ParticleSandboxExportHeatGradientToFile("assets/heat_color_ramp.generated.png");
					Handled = true;
					break;
				case SDL_SCANCODE_R:
					ParticleSandboxReset();
					Handled = true;
					break;
				case SDL_SCANCODE_S:
					ParticleSandboxToggleSpawnersEnabled();
					Handled = true;
					break;
			}
			break;
	}

	return Handled;
}