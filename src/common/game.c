#include "game.h"

#include <HandmadeMath.h>
#include <SDL2/SDL.h>
#include <cimgui.h>
#include <cimgui_impl.h>

#include "Assets.h"
#include "Draw.h"

enum SpriteSheetId {
	SpriteSheetId_Default,
	SpriteSheetId_Ships,
	SpriteSheetId_Projectiles,
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
} CometShip;

typedef struct Projectile {
	uint32 Flags;
	Vec2 Position;
	Vec2 Velocity;
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

struct {
	bool IsRunning;
	SDL_Window* Window;
	SDL_Renderer* Renderer;
	struct {
		ImGuiIO* IO;
	} ImGui;
	GameInput Input;
	ImageAsset* ImageAssets[16];
	int32 Frame;
	GameState State;
} GGame;

bool GameInitialize(const GameInitParams* params)
{
	GGame.IsRunning = true;
	GGame.Window = params->Window;
	GGame.Renderer = SDL_CreateRenderer(GGame.Window, -1, SDL_RENDERER_ACCELERATED);
	SDL_RenderSetLogicalSize(GGame.Renderer, 320, 180);

	igCreateContext(NULL);
	GGame.ImGui.IO = igGetIO();
	GGame.ImGui.IO->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	igStyleColorsDark(NULL);

	ImGui_ImplSDL2_InitForSDLRenderer(GGame.Window, GGame.Renderer);
	ImGui_ImplSDLRenderer_Init(GGame.Renderer);

	AssetsInitialize(&(AssetsConfig){});

	GetImage(SpriteSheetId_Default) = LoadImageAsset("assets/sprite_sheet.png");
	GetImage(SpriteSheetId_Ships) = LoadImageAsset("assets/shipsheet.png");
	GetImage(SpriteSheetId_Projectiles) = LoadImageAsset("assets/projectilesheet.png");
	GetImage(SpriteSheetId_BGObjects0) = LoadImageAsset("assets/CelestialObjects.png");

	DrawInitialize(&(DrawConfig){
		.Renderer = GGame.Renderer,
		.SpriteSheets =
			{
				[SpriteSheetId_Default] = CreateSpriteSheet(GetImage(SpriteSheetId_Default), 8, 8),
				[SpriteSheetId_Ships] = CreateSpriteSheet(GetImage(SpriteSheetId_Ships), 48, 48),
				[SpriteSheetId_Projectiles] =
					CreateSpriteSheet(GetImage(SpriteSheetId_Projectiles), 32, 32),
				[SpriteSheetId_BGObjects0] =
					CreateSpriteSheet(GetImage(SpriteSheetId_BGObjects0), 32, 32),
			},
	});

	GGame.State = (GameState){
		.CometShips =
			{
				[0] =
					{
						.Position = {144.0f, 96.0f},
					},
			},
	};

	return true;
}

void GameDestroy(void)
{
	DrawShutdown();
	AssetsShutdown();
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

	if (GGame.Frame % 6 == 0) {
		real32 yy[] = {-0.1f, -0.05f, 0.0f, 0.05f, 0.1f};
		real32 speed = 200.0f;
		for (int32 i = 0; i < ARRAY_COUNT(yy); i++) {
			CreateProjectile(
				State,
				&(Projectile){
					.Position = State->CometShips[0].Position,
					.Velocity = {CosF(yy[i]) * speed, SinF(yy[i]) * speed},
					.SecondsRemaining = 2.5f,
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

	igShowDemoWindow(NULL);

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

	DrawSprite(&(SpriteDraw){
		.SpriteSheetId = SpriteSheetId_BGObjects0,
		.SpriteId = 44,
		.Position = {170.0f, 20.0f},
		.SpriteTiles = {4, 4},
	});

	DrawSprite(&(SpriteDraw){
		.SpriteId = 16 + ((GGame.Frame % 8) / 4) * 2,
		.Position = GGame.State.CometShips[0].Position,
		.SpriteTiles = {2, 1},
	});

	for (Projectile* Iter = FixedListBegin(GGame.State.Projectiles);
		 Iter != FixedListEnd(GGame.State.Projectiles);
		 Iter++)
	{
		DrawSprite(&(SpriteDraw){
			.SpriteId = 8,
			.Position = Iter->Position,
		});
	}

	DrawRender();

	igRender();
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
