#include "Game.h"

#include <SDL2/SDL.h>
#include <cimgui.h>
#include <cimgui_impl.h>
#include <stdlib.h>
#include <stb_ds.h>

#include "AssetTypes.h"
#include "Debug.h"
#include "Draw.h"
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
	flt32 Rotation;
	int32 SpriteId;
} CometShip;

typedef struct Projectile {
	uint32 Flags;
	Vec2 Position;
	Vec2 Velocity;
	flt32 Facing;
	flt32 SecondsRemaining;
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
	GameInput LastInput;
	ImageAsset* ImageAssets[16];
	SpriteSheetAsset* ShipObjectsSheet;
	int32 Frame;
	GameState State;
	flt32 Timer;
	ImageAsset* HeatRampImage;
	uint32 HeatRampColors[256];
	int32 HeatRampCount;
	rnd_pcg_t RandomGen;
	PhysWorld* Physics;
} GGame;

int32 ExplosionsSpriteIds[11] = {0};

StringId GProjectileSpriteName;

struct BoxBody {
	PolygonShape Box;
	Tform2 XForm;
	flt32 Angle;
};

struct CircBody {
	CircleShape Circ;
	Tform2 XForm;
};

static struct BoxBody GBoxBodies[2];
static struct CircBody GCircBodies[2];
static bool GBoxAABBOverlap = false;
static bool GBoxContact = false;
static bool GCircAABBOverlap = false;
static bool GCircContact = false;
static bool GCircBoxContact0 = false;

bool GameInitialize(const GameInitParams* params)
{
	uint32 RandomSeed = (uint32)SDL_GetPerformanceCounter();
	rnd_pcg_seed(&GGame.RandomGen, RandomSeed);
	uint64 HashtableSeed = (uint64)rnd_pcg_next(&GGame.RandomGen) | (((uint64)rnd_pcg_next(&GGame.RandomGen)) << 32);
	stbds_rand_seed(HashtableSeed);

	LoggingInitialize();
	StringIdPoolsInitialize();

	LogInfo("Game Initializing");

	GGame.Physics = PhysCreateWorld(&(PhysWorldConfig){});

	GProjectileSpriteName = GetStringId("projectile01-1");

	int32 GameResWidth = 576;  // 576;
	int32 GameResHeight = 324; // 324;

	int32 testData[] = { 1, 1, 2, 2, 2, 3, 5, 7, 8, 8, 8, 9, 10, 100, 102, 104, 104, 104 };
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

	GGame.HeatRampImage = (ImageAsset*)LoadAsset(AssetType_Image, "assets/heat_color_ramp.png");
	GGame.HeatRampCount = MIN(GGame.HeatRampImage->Data->Surface->w, ARRAY_COUNT(GGame.HeatRampColors));
	for (int I = 0; I < GGame.HeatRampCount; I++) {
		GGame.HeatRampColors[I] = *((uint32*)GGame.HeatRampImage->Data->Surface->pixels + I);
	}

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

	int32 PlayerSpriteIndex = FindSpriteByName(GGame.ShipObjectsSheet->Data, GetStringId("purple_06"));

	for (int32 Index = 0; Index < ARRAY_COUNT(ExplosionsSpriteIds); Index++) {
		char buffer[64];
		SDL_snprintf(buffer, 64, "explosion-%02d", Index + 1);
		ExplosionsSpriteIds[Index] =
			SPRITE_ID(SpriteSheetId_ShipObjects, FindSpriteByName(GGame.ShipObjectsSheet->Data, GetStringId(buffer)));
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

	PolygonMakeBox(&GBoxBodies[0].Box, V2(24.0f, 12.0f), V2(0, 0), 0.0f);
	GBoxBodies[0].XForm = T2(V2(144, 108), R2Ident());
	PolygonMakeBox(&GBoxBodies[1].Box, V2(12.0f, 24.0f), V2(0, 0), 0.0f);
	GBoxBodies[1].XForm = T2(V2(144 * 3, 108), R2Ident());

	GCircBodies[0].Circ = (CircleShape){.Radius = 18};
	GCircBodies[0].XForm = T2(V2(144, 144), R2Ident());
	GCircBodies[1].Circ = (CircleShape){.Radius = 26};
	GCircBodies[1].XForm = T2(V2(144 * 3, 144), R2Ident());

	LogInfo("Game Initialization Complete");

	return true;
}

void GameShutdown(void)
{
	DrawShutdown();
	AssetsShutdown();
	DebugShutdown();
	PhysDestroyWorld(GGame.Physics);
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

	GameState* State = &GGame.State;

	{
		const float KSpeed = 64.0f;
		Vec2 MoveInput = InputXY(SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN);
		Vec2 Delta = Mul(MoveInput, KSpeed * gameTime->DeltaTimeF);
		State->CometShips[0].Position = Add(State->CometShips[0].Position, Delta);
		State->CometShips[0].Rotation += gameTime->DeltaTimeF;
	}

	static bool control_circs = false;
	if (InputKeyDown(SDL_SCANCODE_T)) control_circs = !control_circs;

	Vec3 What = V3(1.0f, 2.0f, 3.0f);
	Vec4 DaFuq = V4V(What, 10.0f);

	if (control_circs) {
		Vec2 MoveInput0 = InputXY(SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_W, SDL_SCANCODE_S);
		GCircBodies[0].XForm.Position = Add(GCircBodies[0].XForm.Position, MoveInput0);
		Vec2 MoveInput1 = InputXY(SDL_SCANCODE_J, SDL_SCANCODE_L, SDL_SCANCODE_I, SDL_SCANCODE_K);
		GCircBodies[1].XForm.Position = Add(GCircBodies[1].XForm.Position, MoveInput1);
	} else {
		Vec2 MoveInput0 = InputXY(SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_W, SDL_SCANCODE_S);
		GBoxBodies[0].XForm.Position = Add(GBoxBodies[0].XForm.Position, MoveInput0);
		flt32 RotateInput0 = InputAxis(SDL_SCANCODE_Q, SDL_SCANCODE_E);
		GBoxBodies[0].Angle += RotateInput0 * gameTime->DeltaTimeF * KUnitFullTurn32;
		GBoxBodies[0].XForm.Rotation = R2(GBoxBodies[0].Angle);

		Vec2 MoveInput1 = InputXY(SDL_SCANCODE_J, SDL_SCANCODE_L, SDL_SCANCODE_I, SDL_SCANCODE_K);
		GBoxBodies[1].XForm.Position = Add(GBoxBodies[1].XForm.Position, MoveInput1);
		flt32 RotateInput1 = InputAxis(SDL_SCANCODE_U, SDL_SCANCODE_O);
		GBoxBodies[1].Angle += RotateInput1 * gameTime->DeltaTimeF * KUnitFullTurn32;
		GBoxBodies[1].XForm.Rotation = R2(GBoxBodies[1].Angle);
	}

	GBoxAABBOverlap = AABBTestOverlap(
		PolygonCalcAABB(&GBoxBodies[0].Box, GBoxBodies[0].XForm),
		PolygonCalcAABB(&GBoxBodies[1].Box, GBoxBodies[1].XForm));
	GBoxContact =
		GBoxAABBOverlap &&
		PolygonIntersectsPolygon(&GBoxBodies[0].Box, GBoxBodies[0].XForm, &GBoxBodies[1].Box, GBoxBodies[1].XForm);

	GCircAABBOverlap = AABBTestOverlap(
		CircleCalcAABB(&GCircBodies[0].Circ, GCircBodies[0].XForm),
		CircleCalcAABB(&GCircBodies[1].Circ, GCircBodies[1].XForm));
	GCircContact =
		GCircAABBOverlap &&
		CircleIntersectsCircle(&GCircBodies[0].Circ, GCircBodies[0].XForm, &GCircBodies[1].Circ, GCircBodies[1].XForm);

	GCircBoxContact0 =
		CircleIntersectsPolygon(&GCircBodies[0].Circ, GCircBodies[0].XForm, &GBoxBodies[0].Box, GBoxBodies[0].XForm);
#if 0
	if (GGame.Frame % 17 == 0) {
		flt32 yy[] = {-0.1f, -0.05f, 0.0f, 0.05f, 0.1f};
		flt32 speed = 200.0f;
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
#endif

	for (Projectile* Iter = FixedListBegin(GGame.State.Projectiles); Iter != FixedListEnd(GGame.State.Projectiles);
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

	for (Projectile* Iter = FixedListLast(GGame.State.Projectiles); Iter >= FixedListBegin(GGame.State.Projectiles);
		 Iter--)
	{
		if ((Iter->Flags & EntityFlags_Destroyed) != 0) {
			DestroyProjectile(&GGame.State, Iter);
		}
	}

	DebugPrintf("Pos: %0.1f, %0.1f", GGame.State.CometShips[0].Position.X, GGame.State.CometShips[0].Position.Y);
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
	// SDL_SetRenderDrawColor(GGame.Renderer, 0xCC, 0xCC, 0xCC, 255);
	SDL_SetRenderDrawColor(GGame.Renderer, 0, 0, 0, 255);
	SDL_RenderFillRect(GGame.Renderer, &BgRect);

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
		.Rotation = GGame.State.CometShips[0].Rotation,
	});

	// Cycling through big sprite sheet
	DrawSprite(&(SpriteDraw){
		.Position = V2(270, 90),
		.SpriteId =
			SPRITE_ID(SpriteSheetId_ShipObjects, (GGame.Frame / 15) % GGame.ShipObjectsSheet->Data->Frames.Count),
	});

	int32 SpriteIndex = FindSpriteByName(GGame.ShipObjectsSheet->Data, GProjectileSpriteName);
	// Projectiles
	for (Projectile* Iter = FixedListBegin(GGame.State.Projectiles); Iter != FixedListEnd(GGame.State.Projectiles);
		 Iter++)
	{
		DrawSprite(&(SpriteDraw){
			.SpriteId = SPRITE_ID(SpriteSheetId_ShipObjects, SpriteIndex),
			.Position = Iter->Position,
			.Rotation = Iter->Facing + 0.25f,
		});
	}

	uint32 BoxAABBColor = (!GBoxAABBOverlap) ? 0xFFCCCC00 : 0xFFCC00CC;
	uint32 PolygonColor = (!GBoxContact) ? 0xFF00CC00 : 0xFFCC0000;
	for (int32 BodyIndex = 0; BodyIndex < ARRAY_COUNT(GBoxBodies); BodyIndex++) {
		if (BodyIndex == 0 && GCircBoxContact0) PolygonColor = 0xFF00FFFF;
		AABB PolygonAABB = PolygonCalcAABB(&GBoxBodies[BodyIndex].Box, GBoxBodies[BodyIndex].XForm);
		DrawAABB(PolygonAABB, BoxAABBColor);
		DrawPolygon(
			GBoxBodies[BodyIndex].XForm.Position,
			GBoxBodies[BodyIndex].XForm.Rotation,
			GBoxBodies[BodyIndex].Box.Vertices,
			GBoxBodies[BodyIndex].Box.VertexCount,
			PolygonColor);
	}

	uint32 CircAABBColor = (!GCircAABBOverlap) ? 0xFFCCCC00 : 0xFFCC00CC;
	uint32 CircleColor = (!GCircContact) ? 0xFF00CC00 : 0xFFCC0000;
	for (int32 BodyIndex = 0; BodyIndex < ARRAY_COUNT(GCircBodies); BodyIndex++) {
		if (BodyIndex == 0 && GCircBoxContact0) CircleColor = 0xFF00FFFF;
		AABB CircAABB = CircleCalcAABB(&GCircBodies[BodyIndex].Circ, GCircBodies[BodyIndex].XForm);
		DrawAABB(CircAABB, CircAABBColor);
		DrawCircle(
			TransformV2(GCircBodies[BodyIndex].XForm, GCircBodies[BodyIndex].Circ.Center),
			GCircBodies[BodyIndex].Circ.Radius,
			CircleColor);
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
