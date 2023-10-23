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
	SpriteSheetAsset* SpriteSheetAssets[16];
	int32 Frame;
	GameState State;
	real32 Timer;
	ImageAsset* HeatRampImage;
	uint32 HeatRampColors[256];
	int32 HeatRampCount;
} GGame;

StringId GProjectileSpriteName;

bool GameInitialize(const GameInitParams* params)
{
	StringIdPoolsInitialize();

	GProjectileSpriteName = GetStringId("projectile01-1");

	int32 GameResWidth = 576;
	int32 GameRestHeight = 324;

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

	GGame.SpriteSheetAssets[0] = (SpriteSheetAsset*)LoadAsset(
		AssetType_SpriteSheetData, "assets/spritesheets/ship_objects/ship_objects.json");

	DrawInitialize(&(DrawConfig){
		.Renderer = GGame.Renderer,
		.SpriteSheets =
			{
				[SpriteSheetId_Default] =
					CreateSpriteSheetGrid(GetImage(SpriteSheetId_Default), 8, 8),
				[SpriteSheetId_ShipObjects] = CreateSpriteSheetFrameData(
					GetImage(SpriteSheetId_ShipObjects), GGame.SpriteSheetAssets[0]),
				[SpriteSheetId_BGObjects0] =
					CreateSpriteSheetGrid(GetImage(SpriteSheetId_BGObjects0), 32, 32),
			},
	});

	int32 PlayerSpriteIndex =
		FindSpriteByName(GGame.SpriteSheetAssets[0]->Data, GetStringId("purple_06"));

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

	PhysicsInitialize();

	real32 Radius = 4.0f;
	for (real32 PosY = 324 - Radius; PosY > 100; PosY -= Radius * 4) {
		for (real32 PosX = Radius; PosX <= 576 - Radius; PosX += Radius * 3) {
			PhysicsAddObject(&(VertletObject){
				.Position = V2(PosX + (rand() % 10 - 5), PosY),
				.Radius = Radius,
			});
		}
	}

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
	GGame.Timer -= gameTime->DeltaTimeF;
	static Vec2 LastForce = (Vec2){0};
	if (GGame.Timer <= 0.0f) {
		GGame.Timer += 0.25f;

		if (PhysicsGetObjectCount() < 800) {
			real32 Hue = gameTime->ElapsedSeconds / 2.0f;
			uint32 Color = HsvToArgb8888(Hue, 1.0f, 1.0f);
			VertletObject* Object = PhysicsAddObject(&(VertletObject){
				.Position = V2(288.0f, 10.0f),
				.Radius = 8.0f,
			});

			real32 Angle = SinF(gameTime->ElapsedSeconds / 2.0f) * 0.3f + 0.25f;
			real32 Force = 100000.0f;
			Vec2 ForceVec = V2(CosF(Angle) * Force, CosF(Angle) * Force * 0.75f);
			LastForce = ForceVec;
			VertletObjectAccelerate(Object, ForceVec);
		}
	}

	PhysicsUpdate(gameTime->DeltaTimeF);

	DebugPrintf("FPS: %d", (int)round(1.0 / gameTime->DeltaTime));
	DebugPrintf("Objects: %llu", PhysicsGetObjectCount());
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
	SDL_SetRenderDrawColor(GGame.Renderer, 0x12, 0x20, 0x20, 255);
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
		size_t ObjectCount = 0;
		const VertletObject* Objects = PhysicsGetObjects();
		for (size_t Index = 0; Index < PhysicsGetObjectCount(); Index++) {
			const VertletObject* Object = &Objects[Index];
			Vec2 Pos = Object->Position;
			real32 Radius = Object->Radius;
			SDL_FRect PosRect = {Pos.X - Radius, Pos.Y - Radius, Radius * 2 + 1, Radius * 2 + 1};

			uint32 HeatColor = GGame.HeatRampColors[(int32)(MIN(Object->Heat, 1.0f - KEpsilon32) *
															GGame.HeatRampCount)];
			uint R = (HeatColor >> 0) & 0xFF;
			uint G = (HeatColor >> 8) & 0xFF;
			uint B = (HeatColor >> 16) & 0xFF;
			uint A = (HeatColor >> 24);

			SDL_SetRenderDrawColor(GGame.Renderer, R, G, B, A);
			SDL_RenderFillRectF(GGame.Renderer, &PosRect);
		}
	}

	DrawRender();

	DebugDraw(GGame.Renderer);
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