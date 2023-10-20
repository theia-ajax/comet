#include "game.h"

#include <SDL2/SDL.h>
#include <cimgui.h>
#include <cimgui_impl.h>
#include <math.h>

#include "Assets.h"
#include "Draw.h"

enum {
	KMaxProjectiles = 256,
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
	real32 Position[2];
} CometShip;

typedef struct Projectile {
	uint32 Flags;
	real32 Position[2];
	real32 Velocity[2];
	real32 SecondsRemaining;
	Affiliation Affiliation;
} Projectile;

typedef struct GameState {
	CometShip CometShips[1];
	Projectile Projectiles[KMaxProjectiles];
	int32 _ProjectileCount;
} GameState;

Projectile* CreateProjectile(GameState* gameState, const Projectile* config);
void DestroyProjectile(GameState* gameState, Projectile* projectile);

struct {
	bool IsRunning;
	SDL_Window* Window;
	SDL_Renderer* Renderer;
	struct {
		ImGuiIO* IO;
	} ImGui;
	GameInput Input;
	ImageAsset* SpriteSheet;
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

	GGame.SpriteSheet = LoadImageAsset("assets/sprite_sheet.png");

	DrawInitialize(&(DrawConfig){
		.Renderer = GGame.Renderer,
		.SpriteSheets =
			{
				[0] = CreateSpriteSheet(GGame.SpriteSheet, 8, 8),
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
		float MoveInput[2] = { 0 };
		if (GGame.Input.KeyStates[SDL_SCANCODE_LEFT]) MoveInput[0] -= 1.0f;
		if (GGame.Input.KeyStates[SDL_SCANCODE_RIGHT]) MoveInput[0] += 1.0f;
		if (GGame.Input.KeyStates[SDL_SCANCODE_UP]) MoveInput[1] -= 1.0f;
		if (GGame.Input.KeyStates[SDL_SCANCODE_DOWN]) MoveInput[1] += 1.0f;

		const float KSpeed = 64.0f;
		State->CometShips[0].Position[0] += MoveInput[0] * KSpeed * gameTime->DeltaTimeF;
		State->CometShips[0].Position[1] += MoveInput[1] * KSpeed * gameTime->DeltaTimeF;
	}

	if (GGame.Frame % (120) == 0) {
		CreateProjectile(
			State,
			&(Projectile){
				.Position = {State->CometShips[0].Position[0], State->CometShips[0].Position[1]},
				.Velocity = {100.0f, 0.0f},
				.SecondsRemaining = 1.0f,
			});
	}

	for (int32 ProjectileIndex = 0; ProjectileIndex < State->_ProjectileCount; ProjectileIndex++) {
		Projectile* ProjectileIter = &State->Projectiles[ProjectileIndex];
		ProjectileIter->Position[0] += ProjectileIter->Velocity[0] * gameTime->DeltaTimeF;
		ProjectileIter->Position[1] += ProjectileIter->Velocity[1] * gameTime->DeltaTimeF;

		if (ProjectileIter->SecondsRemaining > 0.0f) {
			ProjectileIter->SecondsRemaining -= gameTime->DeltaTimeF;
			if (ProjectileIter->SecondsRemaining <= 0.0f) {
				ProjectileIter->Flags |= EntityFlags_Destroyed;
			}
		}
	}

	for (int32 ProjectileIndex = State->_ProjectileCount - 1; ProjectileIndex >= 0;
		 ProjectileIndex--)
	{
		Projectile* Projectile_ = &State->Projectiles[ProjectileIndex];
		if ((Projectile_->Flags & EntityFlags_Destroyed) != 0) {
			DestroyProjectile(&GGame.State, Projectile_);
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

	SDL_Rect BgRect= {
		0, 0, RenderWidth, RenderHeight
	};
	SDL_SetRenderDrawColor(GGame.Renderer, 0x12, 0x20, 0x20, 255);
	SDL_RenderFillRect(GGame.Renderer, &BgRect);

	DrawSprite(&(SpriteDraw){
		.SpriteId = 16 + ((GGame.Frame % 8) / 4) * 2,
		.Position[0] = GGame.State.CometShips[0].Position[0],
		.Position[1] = GGame.State.CometShips[0].Position[1],
		.SpriteTiles = {2, 1},
	});

	for (Projectile* Iter = GGame.State.Projectiles;
		 Iter != GGame.State.Projectiles + GGame.State._ProjectileCount;
		 Iter++)
	{
		SpriteDraw DrawCommand = (SpriteDraw){
			.SpriteId = 8,
		};
		memcpy(DrawCommand.Position, Iter->Position, sizeof(DrawCommand.Position));

		DrawSprite(&DrawCommand);
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

Projectile* CreateProjectile(GameState* gameState, const Projectile* config)
{
	ASSERT(gameState);
	ASSERT(gameState->_ProjectileCount < KMaxProjectiles);

	Projectile* Result = &gameState->Projectiles[gameState->_ProjectileCount++];
	*Result = (config != NULL) ? *config : (Projectile){};

	return Result;
}

void DestroyProjectile(GameState* gameState, Projectile* projectile)
{
	ASSERT(gameState);
	ASSERT(projectile);

	ptrdiff_t ProjectileIndex = projectile - &gameState->Projectiles[0];

	ASSERT(VALID_INDEX(ProjectileIndex, gameState->_ProjectileCount));

	gameState->_ProjectileCount--;
	gameState->Projectiles[ProjectileIndex] = gameState->Projectiles[gameState->_ProjectileCount];
}
