#include "Game.h"

#include <SDL2/SDL.h>
#include <cimgui.h>
#include <cimgui_impl.h>
#include <stdlib.h>

#include "AssetTypes.h"
#include "Debug.h"
#include "Draw.h"
#include "Math.h"
#include "Physics.h"
#include "Random.h"
#include "StringId.h"

enum SpriteSheetId {
	SpriteSheetId_Default,
	SpriteSheetId_ShipObjects,
	SpriteSheetId_BGObjects0,
	SpriteSheetId_Count,
};

enum {
	KMaxProjectiles = 4096,
};

enum EntityFlags {
	EntityFlags_None = 0,
	EntityFlags_Destroyed = 1 << 0,
};

typedef enum Affiliation {
	Affiliation_Friendly,
	Affiliation_Neutral,
	Affiliation_Hostile,
	Affiliation_Count,
} Affiliation;

typedef struct CometShip {
	Vec2 Position;
	int32 SpriteId;
} CometShip;

typedef struct Projectile {
	uint32 Flags;
	Vec2 Position;
	Vec2 Velocity;
	real32 Facing;
	real32 SecondsRemaining;
	Affiliation Affiliation;
} Projectile;

typedef struct GameState {
	CometShip CometShips[1];
	FixedList(Projectile, KMaxProjectiles) Projectiles;
	// Projectile Projectiles[KMaxProjectiles];
	// int32 _ProjectileCount;
} GameState;

#define GetImage(Id) GGame.ImageAssets[Id]
static Projectile* CreateProjectile(GameState* gameState, const Projectile* config);
static void DestroyProjectile(GameState* gameState, Projectile* projectile);
static uint32 HsvToArgb8888(real32 H, real32 S, real32 V);

struct {
	bool IsRunning;
	SDL_Window* Window;
	SDL_Renderer* Renderer;
	struct {
		ImGuiIO* IO;
	} ImGui;
	GameInput Input;
	ImageAsset* ImageAssets[16];
	SpriteSheetAsset* ShipObjectsSheet;
	int32 Frame;
	GameState State;
	real32 Timer;
	ImageAsset* HeatRampImage;
	uint32 HeatRampColors[256];
	int32 HeatRampCount;
	rnd_pcg_t RandomGen;
	PhysicsConstraintHandle MousePinConstraint;
} GGame;

int32 ExplosionsSpriteIds[11] = {0};

StringId GProjectileSpriteName;

bool GameInitialize(const GameInitParams* params)
{
	StringIdPoolsInitialize();

	Uint64 PerformanceCounter = SDL_GetPerformanceCounter();
	rnd_pcg_seed(&GGame.RandomGen, (uint32)PerformanceCounter);

	GProjectileSpriteName = GetStringId("projectile01-1");

	int32 GameResWidth = 576;	// 576;
	int32 GameRestHeight = 324; // 324;

	GGame.IsRunning = true;
	GGame.Window = params->Window;
	GGame.Renderer = SDL_CreateRenderer(GGame.Window, -1, SDL_RENDERER_ACCELERATED);
	SDL_RenderSetLogicalSize(GGame.Renderer, GameResWidth, GameRestHeight);

	igCreateContext(NULL);
	GGame.ImGui.IO = igGetIO();
	GGame.ImGui.IO->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	igStyleColorsDark(NULL);

	DebugInitialize(&(DebugConfig){
		.CanvasWidth = GameResWidth,
		.CanvasHeight = GameRestHeight,
	});

	ImGui_ImplSDL2_InitForSDLRenderer(GGame.Window, GGame.Renderer);
	ImGui_ImplSDLRenderer_Init(GGame.Renderer);

	AssetsInitialize(&(AssetsConfig){
		.TypeConfigs = {
			[AssetType_Image] =
				{.LoadAssetData = (LoadAssetDataFunc)LoadImageData,
				 .UnloadAssetData = (UnloadAssetDataFunc)UnloadImageData,
				 .Size = sizeof(ImageData)},
			[AssetType_SpriteSheetData] =
				{.LoadAssetData = (LoadAssetDataFunc)LoadSpriteSheetData,
				 .UnloadAssetData = (UnloadAssetDataFunc)UnloadSpriteSheetData,
				 .Size = sizeof(SpriteSheetData)},
		}});

	GGame.HeatRampImage = (ImageAsset*)LoadAsset(AssetType_Image, "assets/heat_color_ramp.png");
	GGame.HeatRampCount =
		MIN(GGame.HeatRampImage->Data->Surface->w, ARRAY_COUNT(GGame.HeatRampColors));
	for (int I = 0; I < GGame.HeatRampCount; I++) {
		GGame.HeatRampColors[I] = *((uint32*)GGame.HeatRampImage->Data->Surface->pixels + I);
	}

	GetImage(SpriteSheetId_Default) =
		(ImageAsset*)LoadAsset(AssetType_Image, "assets/sprite_sheet.png");
	GetImage(SpriteSheetId_ShipObjects) = (ImageAsset*)LoadAsset(
		AssetType_Image, "assets/spritesheets/ship_objects/ship_objects.png");
	GetImage(SpriteSheetId_BGObjects0) =
		(ImageAsset*)LoadAsset(AssetType_Image, "assets/CelestialObjects.png");

	GGame.ShipObjectsSheet = (SpriteSheetAsset*)LoadAsset(
		AssetType_SpriteSheetData, "assets/spritesheets/ship_objects/ship_objects.json");

	DrawInitialize(&(DrawConfig){
		.Renderer = GGame.Renderer,
		.SpriteSheets =
			{
				[SpriteSheetId_Default] =
					CreateSpriteSheetGrid(GetImage(SpriteSheetId_Default), 8, 8),
				[SpriteSheetId_ShipObjects] = CreateSpriteSheetFrameData(
					GetImage(SpriteSheetId_ShipObjects), GGame.ShipObjectsSheet),
				[SpriteSheetId_BGObjects0] =
					CreateSpriteSheetGrid(GetImage(SpriteSheetId_BGObjects0), 32, 32),
			},
	});

	int32 PlayerSpriteIndex =
		FindSpriteByName(GGame.ShipObjectsSheet->Data, GetStringId("purple_06"));

	for (int32 Index = 0; Index < ARRAY_COUNT(ExplosionsSpriteIds); Index++) {
		char buffer[64];
		SDL_snprintf(buffer, 64, "explosion-%02d", Index + 1);
		ExplosionsSpriteIds[Index] = SPRITE_ID(
			SpriteSheetId_ShipObjects,
			FindSpriteByName(GGame.ShipObjectsSheet->Data, GetStringId(buffer)));
	}

	GGame.State = (GameState){
		.CometShips =
			{
				[0] =
					{
						.Position = {144.0f, 96.0f},
						.SpriteId = SPRITE_ID(SpriteSheetId_ShipObjects, PlayerSpriteIndex),
					},
			},
	};

	real32 CellSize = 2.0f;
	PhysicsConfig PhysicsCfg = PhysicsDefaultConfig();
	PhysicsCfg.CellSize = CellSize;
	PhysicsCfg.Bounds.XY = V2(-CellSize, -CellSize);
	PhysicsCfg.Bounds.ZW = V2(GameResWidth + CellSize, GameRestHeight + CellSize);
	PhysicsInitialize(&PhysicsCfg);

	PhysicsObjectHandle HPinObject = PhysicsAddObject(&(PhysicsObject){
		.Flags = 1,
		.Radius = 1.0f,
		.Tint = 0xFF00FFFF,
	});
	GGame.MousePinConstraint =
		PhysicsAddPinConstraint(HPinObject, V2(GameResWidth / 2, GameRestHeight / 2));

	{
		PhysicsObjectHandle LastHandle = {KInvalidHandle};
		for (int i = 0; i < 20; i++) {
			real32 Radius = 2.0f;
			PhysicsObjectHandle NewHandle = PhysicsAddObject(&(PhysicsObject){
				.Radius = Radius,
				.Position = V2(GameResWidth / 2.0f, 20.0f + Radius * 2 * i),
				.Flags = 1,
				.Tint = 0xFFFFFFF,
			});

			if (LastHandle.Value != KInvalidHandle) {
				PhysicsAddLinkConstraint(NewHandle, LastHandle, Radius * 2);
			} else {
				PhysicsAddPinConstraint(NewHandle, PhysicsGetObject(NewHandle)->Position);
			}

			LastHandle = NewHandle;
		}
		PhysicsAddPinConstraint(LastHandle, PhysicsGetObject(LastHandle)->Position);
	}
	// {
	// 	PhysicsObjectHandle LastHandle = {KInvalidHandle};
	// 	for (int i = 0; i < 20; i++) {
	// 		real32 Radius = 16.0f;
	// 		PhysicsObjectHandle NewHandle = PhysicsAddObject(&(PhysicsObject){
	// 			.Radius = Radius,
	// 			.Position = V2(3.0f * GameResWidth / 4.0f, 200.0f + Radius * 2 * i),
	// 			.Flags = 1,
	// 			.Tint = 0xFFFFFFF,
	// 		});

	// 		if (LastHandle.Value != KInvalidHandle) {
	// 			PhysicsAddLinkConstraint(NewHandle, LastHandle, Radius * 2);
	// 		} else {
	// 			PhysicsAddPinConstraint(NewHandle, PhysicsGetObject(NewHandle)->Position);
	// 		}

	// 		LastHandle = NewHandle;
	// 	}
	// }

	// real32 Spacing = 5.0f;
	// for (real32 PosY = GameRestHeight - Spacing; PosY > GameRestHeight - 600; PosY -= Spacing
	// * 2.5f)
	// {
	// 	for (real32 PosX = Spacing; PosX <= GameResWidth - Spacing; PosX += Spacing * 2) {
	// 		real32 Radius = 4.0f;

	// 		PhysicsAddObject(&(PhysicsObject){
	// 			.Position = V2(PosX + (rand() % 10 - 5), PosY),
	// 			.Radius = Radius,
	// 		});
	// 	}
	// }

	// real32 Spacing = 5.0f;
	// for (real32 PosY = PhysicsCfg.Bounds.W - Spacing; PosY > PhysicsCfg.Bounds.Y + Spacing;
	// 	 PosY -= Spacing * 2.0f)
	// {
	// 	for (real32 PosX = PhysicsCfg.Bounds.X + Spacing; PosX <= PhysicsCfg.Bounds.Z - Spacing;
	// 		 PosX += Spacing * 2.0f)
	// 	{
	// 		real32 Radius = 1.0f;

	// 		PhysicsAddObject(&(PhysicsObject){
	// 			.Position = V2(PosX, PosY),
	// 			.Radius = Radius,
	// 		});
	// 	}
	// }

	return true;
}

void GameShutdown(void)
{
	PhysicsShutdown();
	DrawShutdown();
	AssetsShutdown();
	DebugShutdown();
	StringIdPoolsShutdown();
}

void GameSendInput(const GameInput* input)
{
	memcpy(&GGame.Input, input, sizeof(GameInput));
}

void GameProcessEvent(const SDL_Event* event)
{
	ImGui_ImplSDL2_ProcessEvent(event);
}

void GameUpdate(const GameTime* gameTime)
{
	GGame.ImGui.IO->DeltaTime = gameTime->DeltaTimeF;
	ImGui_ImplSDLRenderer_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	igNewFrame();

	DebugNextFrame();

#if 0
	GameState* State = &GGame.State;

	{
		Vec2 MoveInput = {0};
		if (GGame.Input.KeyStates[SDL_SCANCODE_LEFT]) MoveInput.X -= 1.0f;
		if (GGame.Input.KeyStates[SDL_SCANCODE_RIGHT]) MoveInput.X += 1.0f;
		if (GGame.Input.KeyStates[SDL_SCANCODE_UP]) MoveInput.Y -= 1.0f;
		if (GGame.Input.KeyStates[SDL_SCANCODE_DOWN]) MoveInput.Y += 1.0f;

		const float KSpeed = 64.0f;
		Vec2 Delta = Mul(MoveInput, KSpeed * gameTime->DeltaTimeF);
		State->CometShips[0].Position = Add(State->CometShips[0].Position, Delta);
	}

	if (GGame.Frame % 17 == 0) {
		real32 yy[] = {-0.1f, -0.05f, 0.0f, 0.05f, 0.1f};
		real32 speed = 200.0f;
		for (int32 i = 0; i < ARRAY_COUNT(yy); i++) {
			CreateProjectile(
				State,
				&(Projectile){
					.Position = AddV2(State->CometShips[0].Position, V2(4.0f, 0.0f)),
					.Velocity = {CosF(yy[i]) * speed, SinF(yy[i]) * speed},
					.SecondsRemaining = 2.5f,
					.Facing = yy[i],
				});
		}
	}

	for (Projectile* Iter = FixedListBegin(GGame.State.Projectiles);
		 Iter != FixedListEnd(GGame.State.Projectiles);
		 Iter++)
	{
		Iter->Position = Add(Iter->Position, Mul(Iter->Velocity, gameTime->DeltaTimeF));

		if (Iter->SecondsRemaining > 0.0f) {
			Iter->SecondsRemaining -= gameTime->DeltaTimeF;
			if (Iter->SecondsRemaining <= 0.0f) {
				Iter->Flags |= EntityFlags_Destroyed;
			}
		}
	}

	for (Projectile* Iter = FixedListLast(GGame.State.Projectiles);
		 Iter >= FixedListBegin(GGame.State.Projectiles);
		 Iter--)
	{
		if ((Iter->Flags & EntityFlags_Destroyed) != 0) {
			DestroyProjectile(&GGame.State, Iter);
		}
	}

	DebugPrintf(
		"Pos: %0.1f, %0.1f",
		GGame.State.CometShips[0].Position.X,
		GGame.State.CometShips[0].Position.Y);
#endif
	// GGame.Timer -= gameTime->DeltaTimeF;
	// static Vec2 LastForce = (Vec2){0};
	// if (GGame.Timer <= 0.0f) {
	// 	GGame.Timer += 0.25f;

	// 	if (PhysicsGetObjectCount() < 800) {
	// 		real32 Hue = gameTime->ElapsedSeconds / 2.0f;
	// 		uint32 Color = HsvToArgb8888(Hue, 1.0f, 1.0f);
	// 		PhysicsObject* Object = PhysicsAddObject(&(PhysicsObject){
	// 			.Position = V2(288.0f, 10.0f),
	// 			.Radius = 8.0f,
	// 		});

	// 		real32 Angle = SinF(gameTime->ElapsedSeconds / 2.0f) * 0.3f + 0.25f;
	// 		real32 Force = 100000.0f;
	// 		Vec2 ForceVec = V2(CosF(Angle) * Force, CosF(Angle) * Force * 0.75f);
	// 		LastForce = ForceVec;
	// 		PhysicsObjectAccelerate(Object, ForceVec);
	// 	}
	// }

	int FramesPerSecond = (int)round(1.0 / gameTime->DeltaTime);

	real32 Spawners[4 * 12] = {
		24,		  100, 100000,	25000, 576 - 24, 100, -100000, 25000, 24,		120, 100000,  25000,
		576 - 24, 120, -100000, 25000, 24,		 140, 100000,  25000, 576 - 24, 140, -100000, 25000,
		24,		  160, 100000,	25000, 576 - 24, 160, -100000, 25000, 24,		180, 100000,  25000,
		576 - 24, 180, -100000, 25000, 24,		 200, 100000,  25000, 576 - 24, 200, -100000, 25000,
	};

	for (int32 SpawnerIndex = 0; SpawnerIndex < ARRAY_COUNT(Spawners); SpawnerIndex += 4) {
		real32* Spawner = &Spawners[SpawnerIndex];
		Vec2 SpawnPos = V2(Spawner[0], Spawner[1]);
		Vec2 SpawnAccel = V2(Spawner[2], Spawner[3]);
		if (gameTime->SimTimeMS < (1000.0 / 60.0) && PhysicsIsAreaClear(SpawnPos)) {
			PhysicsAddObject(&(PhysicsObject){
				.Position = SpawnPos,
				.Radius = 1.0f,
				.Acceleration = SpawnAccel,
				.Heat = 1.0f,
			});
		}
	}

	int MouseX, MouseY;
	SDL_GetMouseState(&MouseX, &MouseY);
	PhysicsGetConstraint(GGame.MousePinConstraint)->Pin.Position = V2(MouseX, MouseY);

	PhysicsUpdate(gameTime->DeltaTimeF);

	DebugPrintf("FPS: %d, SIM: %0.3fms", FramesPerSecond, gameTime->SimTimeMS);
	DebugPrintf("Objects: %llu", PhysicsGetObjectCount());
	PhysicsObject* MouseObject = PhysicsGetPinConstraintObject(GGame.MousePinConstraint);
	if (MouseObject)
		DebugPrintf(
			"Mouse: %0.2f, %0.2f -- %d",
			MouseObject->Position.X,
			MouseObject->Position.Y,
			MouseObject->GridCell);
	// DebugPrintf("LastF: %0.2f, %0.2f", LastForce.X, LastForce.Y);

	igRender();

	GGame.Frame++;
}

void GameRender(const GameTime* gameTime)
{
	SDL_SetRenderDrawColor(GGame.Renderer, 0, 0, 0, 255);
	SDL_RenderClear(GGame.Renderer);

	int RenderWidth, RenderHeight;
	SDL_GetRendererOutputSize(GGame.Renderer, &RenderWidth, &RenderHeight);

	SDL_Rect BgRect = {0, 0, RenderWidth, RenderHeight};
	// SDL_SetRenderDrawColor(GGame.Renderer, 0x12, 0x20, 0x20, 255);
	// SDL_SetRenderDrawColor(GGame.Renderer, 0xCC, 0xCC, 0xCC, 255);
	SDL_SetRenderDrawColor(GGame.Renderer, 0, 0, 0, 255);
	SDL_RenderFillRect(GGame.Renderer, &BgRect);

#if 0
	// Background nebula
	DrawSprite(&(SpriteDraw){
		.SpriteId = SPRITE_ID(SpriteSheetId_BGObjects0, 44),
		.Position = {170.0f, 20.0f},
		.SpriteTiles = {4, 4},
	});

	// Player ship
	DrawSprite(&(SpriteDraw){
		.SpriteId = GGame.State.CometShips[0].SpriteId,
		.Position = GGame.State.CometShips[0].Position,
		.SpriteTiles = {2, 1},
		.Rotation = 0.25f,
	});

	// Cycling through big sprite sheet
	DrawSprite(&(SpriteDraw){
		.Position = V2(270, 90),
		.SpriteId = SPRITE_ID(
			SpriteSheetId_ShipObjects,
			(GGame.Frame / 15) % GGame.SpriteSheetAssets[0]->Data->Frames.Count),
	});

	int32 SpriteIndex = FindSpriteByName(GGame.SpriteSheetAssets[0]->Data, GProjectileSpriteName);
	// Projectiles
	for (Projectile* Iter = FixedListBegin(GGame.State.Projectiles);
		 Iter != FixedListEnd(GGame.State.Projectiles);
		 Iter++)
	{
		DrawSprite(&(SpriteDraw){
			.SpriteId = SPRITE_ID(SpriteSheetId_ShipObjects, SpriteIndex),
			.Position = Iter->Position,
			.Rotation = Iter->Facing + 0.25f,
		});
	}
#endif
	{
		// int32 FireSpriteIndex =
		// 	FindSpriteByName(GGame.ShipObjectsSheet->Data, GetStringId("explosion-04"));
		// int32 FireSpriteId = SPRITE_ID(SpriteSheetId_ShipObjects, FireSpriteIndex);

		size_t ObjectCount = 0;
		const PhysicsObject* Objects = PhysicsGetObjects();
		for (size_t Index = 0; Index < PhysicsGetObjectCount(); Index++) {
			const PhysicsObject* Object = &Objects[Index];
			Vec2 Pos = Object->Position;
			real32 Radius = Object->Radius;
			SDL_FRect PosRect = {Pos.X - Radius, Pos.Y - Radius, Radius * 2 + 1, Radius * 2 + 1};
			uint32 TintColor;
			if ((Object->Flags & 1) != 0) {
				TintColor = Object->Tint;
			} else {
				TintColor = GGame.HeatRampColors[(int32)(MIN(Object->Heat, 1.0f - KEpsilon32) *
														 GGame.HeatRampCount)];
			}

			real32 Scale = (Radius + Object->Heat * 0.1f) / 26.0f;

			DrawSprite(&(SpriteDraw){
				.SpriteId = ExplosionsSpriteIds[3],
				.Position = Pos,
				.Scale = V2(Scale, Scale),
				.UseTint = true,
				.TintColor = TintColor,
			});
			// SDL_SetRenderDrawColor(GGame.Renderer, R, G, B, A);
			// SDL_RenderFillRectF(GGame.Renderer, &PosRect);
		}
	}

	DrawRender();

	DebugDraw(GGame.Renderer);
	// PhysicsDebugDraw(GGame.Renderer);
	ImGui_ImplSDLRenderer_RenderDrawData(igGetDrawData());

	SDL_RenderPresent(GGame.Renderer);
}

bool GameIsRunning(void)
{
	return GGame.IsRunning;
}

void GameRequestShutdown(void)
{
	GGame.IsRunning = false;
}

static Projectile* CreateProjectile(GameState* gameState, const Projectile* config)
{
	ASSERT(gameState);
	ASSERT(!FixedListIsFull(gameState->Projectiles));

	Projectile* Result = FixedListPush(gameState->Projectiles);
	*Result = (config != NULL) ? *config : (Projectile){};

	return Result;
}

static void DestroyProjectile(GameState* gameState, Projectile* projectile)
{
	ASSERT(gameState);
	ASSERT(projectile);

	// ptrdiff_t ProjectileIndex = FixedListIndexOf(gameState->Projectiles, projectile);
	ptrdiff_t ProjectileIndex = projectile - &gameState->Projectiles.Data[0];

	ASSERT(VALID_INDEX(ProjectileIndex, gameState->Projectiles.Count));

	FixedListRemoveAt(gameState->Projectiles, ProjectileIndex);
}

static uint32 HsvToArgb8888(real32 H, real32 S, real32 V)
{
	H = (H >= 0 ? 0.0f : 1.0f) + fmodf(H, 1.0f);
	H *= 360.0f;
	real32 C = V * S;
	real32 X = C * (1.0f - fabs(fmod((H / 60.0f), 2) - 1.0f));
	real32 M = V - C;

	real32 RP = 0.0f, GP = 0.0f, BP = 0.0f;
	if (H < 60) {
		RP = C;
		GP = X;
	} else if (H < 120) {
		RP = X;
		GP = C;
	} else if (H < 180) {
		GP = C;
		BP = X;
	} else if (H < 240) {
		GP = X;
		BP = C;
	} else if (H < 300) {
		RP = X;
		BP = C;
	} else if (H < 360) {
		RP = C;
		BP = X;
	}

	Uint8 R = (Uint8)((RP + M) * 255.0f);
	Uint8 G = (Uint8)((GP + M) * 255.0f);
	Uint8 B = (Uint8)((BP + M) * 255.0f);

	uint32 Result = (255 << 24) | (R << 16) | (G << 8) | B;
	return Result;
}