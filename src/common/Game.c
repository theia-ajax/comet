#include "Game.h"

#include <SDL2/SDL.h>
#include <cimgui.h>
#include <cimgui_impl.h>
#include <stb_ds.h>
#include <stdlib.h>

#include "AssetTypes.h"
#include "Debug.h"
#include "Draw.h"
#include "GameWorld.h"
#include "Log.h"
#include "Math2D.h"
#include "Physics.h"
#include "Random.h"
#include "StringId.h"
#include "Util.h"

enum SpriteSheetId {
	SpriteSheetId_Default,
	SpriteSheetId_ShipObjects,
	SpriteSheetId_BGObjects0,
	SpriteSheetId_Count,
};

#define GetImage(Id) GGame.ImageAssets[Id]

typedef void (GameSystem)(GameWorld* World, const GameTime* Time);

struct {
	bool IsRunning;
	SDL_Window* Window;
	SDL_Renderer* Renderer;
	struct {
		ImGuiIO* IO;
	} ImGui;
	GameInput Input;
	GameInput LastInput;
	ImageAsset* ImageAssets[16];
	SpriteSheetAsset* ShipObjectsSheet;
	int32 Frame;
	rnd_pcg_t RandomGen;
	// PhysWorld* Physics;
	GameWorld* World;
	// GameSystem* Systems[];
} GGame;

bool GameInitialize(const GameInitParams* params)
{
	LoggingInitialize();
	LogInfo(__FUNCTION__);

	uint32 RandomSeed = (uint32)SDL_GetPerformanceCounter();
	rnd_pcg_seed(&GGame.RandomGen, RandomSeed);
	uint64 HashtableSeed = (uint64)rnd_pcg_next(&GGame.RandomGen) | (((uint64)rnd_pcg_next(&GGame.RandomGen)) << 32);
	stbds_rand_seed(HashtableSeed);

	StringIdPoolsInitialize();

	int32 GameResWidth = 512;  // 576;D
	int32 GameResHeight = 288; // 324;

	int32 testData[] = {1, 1, 2, 2, 2, 3, 5, 7, 8, 8, 8, 9, 10, 100, 102, 104, 104, 104};
	LogInfo("%d", BinarySearch(1, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearch(2, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearch(3, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearch(4, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearch(5, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearch(8, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearch(101, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearch(103, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearch(104, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearch(105, testData, ARRAY_COUNT(testData)));
	LogInfo("---");
	LogInfo("%d", BinarySearchInsertIndex(1, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearchInsertIndex(2, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearchInsertIndex(3, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearchInsertIndex(4, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearchInsertIndex(5, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearchInsertIndex(8, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearchInsertIndex(101, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearchInsertIndex(103, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearchInsertIndex(104, testData, ARRAY_COUNT(testData)));
	LogInfo("%d", BinarySearchInsertIndex(105, testData, ARRAY_COUNT(testData)));

	LogInfo("Creating Renderer");
	GGame.IsRunning = true;
	GGame.Window = params->Window;
	GGame.Renderer = SDL_CreateRenderer(GGame.Window, -1, SDL_RENDERER_ACCELERATED);
	SDL_RenderSetLogicalSize(GGame.Renderer, GameResWidth, GameResHeight);

	igCreateContext(NULL);
	GGame.ImGui.IO = igGetIO();
	GGame.ImGui.IO->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	igStyleColorsDark(NULL);

	DebugInitialize(&(DebugConfig){
		.CanvasWidth = GameResWidth,
		.CanvasHeight = GameResHeight,
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

	GetImage(SpriteSheetId_Default) = (ImageAsset*)LoadAsset(AssetType_Image, "assets/sprite_sheet.png");
	GetImage(SpriteSheetId_ShipObjects) =
		(ImageAsset*)LoadAsset(AssetType_Image, "assets/spritesheets/ship_objects/ship_objects.png");
	GetImage(SpriteSheetId_BGObjects0) = (ImageAsset*)LoadAsset(AssetType_Image, "assets/CelestialObjects.png");

	GGame.ShipObjectsSheet =
		(SpriteSheetAsset*)LoadAsset(AssetType_SpriteSheetData, "assets/spritesheets/ship_objects/ship_objects.json");

	DrawInitialize(&(DrawConfig){
		.Renderer = GGame.Renderer,
		.SpriteSheets =
			{
				[SpriteSheetId_Default] = CreateSpriteSheetGrid(GetImage(SpriteSheetId_Default), 8, 8),
				[SpriteSheetId_ShipObjects] =
					CreateSpriteSheetFrameData(GetImage(SpriteSheetId_ShipObjects), GGame.ShipObjectsSheet),
				[SpriteSheetId_BGObjects0] = CreateSpriteSheetGrid(GetImage(SpriteSheetId_BGObjects0), 32, 32),
			},
	});

	LogInfo("Game Systems Initialized");

	GGame.World = CreateGameWorld();

	EntityId Entity0 = CreateEntity(GGame.World);
	EntityId Entity1 = CreateEntity(GGame.World);

	*AddComponent(TransformComponent, GGame.World, Entity0) = (TransformComponent){
		.Position = V2(64.0f, 128.0f),
		.Rotation = 0.25f,
	};

	*AddComponent(SpriteComponent, GGame.World, Entity0) = (SpriteComponent){
		.SpriteId = SPRITE_ID(
			SpriteSheetId_ShipObjects, FindSpriteByName(GGame.ShipObjectsSheet->Data, GetStringId("green_01"))),
	};

	*AddComponent(ColliderComponent, GGame.World, Entity0) = (ColliderComponent){
		.Type = ColliderType_Circle,
		.Circle = {
			.Radius = 36.0f,
		}};

	*AddComponent(TransformComponent, GGame.World, Entity1) = (TransformComponent){
		.Position = V2(256.0f, 128.0f),
		.Rotation = -0.25f,
	};

	*AddComponent(SpriteComponent, GGame.World, Entity1) = (SpriteComponent){
		.SpriteId = SPRITE_ID(
			SpriteSheetId_ShipObjects, FindSpriteByName(GGame.ShipObjectsSheet->Data, GetStringId("red_01"))),
	};

	*AddComponent(ColliderComponent, GGame.World, Entity1) = (ColliderComponent){
		.Type = ColliderType_Circle,
		.Circle = {
			.Radius = 36.0f,
		}};

	return true;
}

void GameShutdown(void)
{
	DrawShutdown();
	LogInfo("Destrying Renderer");
	SDL_DestroyRenderer(GGame.Renderer);
	AssetsShutdown();
	DebugShutdown();
	DestroyGameWorld(GGame.World);
	StringIdPoolsShutdown();

	LogInfo(__FUNCTION__);
	LoggingShutdown();
}

void GameSendInput(const GameInput* input)
{
	memcpy(&GGame.LastInput, &GGame.Input, sizeof(GameInput));
	memcpy(&GGame.Input, input, sizeof(GameInput));
}

void GameProcessEvent(const SDL_Event* event)
{
	ImGui_ImplSDL2_ProcessEvent(event);
}

static inline bool InputKey(int Scancode)
{
	return GGame.Input.KeyStates[Scancode];
}
static inline bool InputKeyDown(int Scancode)
{
	return GGame.Input.KeyStates[Scancode] && !GGame.LastInput.KeyStates[Scancode];
}
static inline bool InputKeyUp(int Scancode)
{
	return !GGame.Input.KeyStates[Scancode] && GGame.LastInput.KeyStates[Scancode];
}

static flt32 InputAxis(int ScancodeNeg, int ScancodePos)
{
	flt32 Axis = 0.0f;
	if (InputKey(ScancodeNeg)) Axis -= 1.0f;
	if (InputKey(ScancodePos)) Axis += 1.0f;
	return Axis;
}

static Vec2 InputXY(int ScancodeLeft, int ScancodeRight, int ScancodeUp, int ScancodeDown)
{
	return (Vec2){
		.X = InputAxis(ScancodeLeft, ScancodeRight),
		.Y = InputAxis(ScancodeUp, ScancodeDown),
	};
}

void GameUpdate(const GameTime* gameTime)
{
	GGame.ImGui.IO->DeltaTime = gameTime->DeltaTimeF;
	ImGui_ImplSDLRenderer_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	igNewFrame();

	DebugNextFrame();

	int FramesPerSecond = (int)round(1.0 / gameTime->DeltaTime);
	DebugPrintf("FPS: %d, SIM: %0.3fms", FramesPerSecond, gameTime->SimTimeMS);

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
	SDL_SetRenderDrawColor(GGame.Renderer, 0xCC, 0xCC, 0xCC, 255);
	// SDL_SetRenderDrawColor(GGame.Renderer, 0, 0, 0, 255);
	SDL_RenderFillRect(GGame.Renderer, &BgRect);

	// Background nebula
	DrawSprite(&(SpriteDraw){
		.SpriteId = SPRITE_ID(SpriteSheetId_BGObjects0, 44),
		.Position = {372.0f, 128.0f},
		.SpriteTiles = {4, 4},
	});

	for (EntityId* Iter = WorldEntitiesBegin(GGame.World); Iter != WorldEntitiesEnd(GGame.World); Iter++) {
		EntityId Entity = *Iter;
		if (HAS_COMPONENTS(GGame.World, Entity, TransformComponent, SpriteComponent))
		{
			TransformComponent* T = GetComponent(TransformComponent, GGame.World, Entity);
			SpriteComponent* S = GetComponent(SpriteComponent, GGame.World, Entity);
			DrawSprite(&(SpriteDraw){
				.SpriteId = S->SpriteId,
				.Position = T->Position,
				.Rotation = T->Rotation,
			});
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
