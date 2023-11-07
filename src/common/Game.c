#include "Game.h"

#include <SDL3/SDL.h>
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

enum {
	Group_Friendly,
	Group_Hostile,
};

enum SpriteSheetId {
	SpriteSheetId_Default,
	SpriteSheetId_ShipObjects,
	SpriteSheetId_BGObjects0,
	SpriteSheetId_Count,
};

#define GetImage(Id) GGame.ImageAssets[Id]

typedef void(GameSystem)(GameWorld* World, const GameTime* Time);

struct {
	bool IsRunning;
	SDL_Window* Window;
	SDL_Renderer* Renderer;
	GameInput Input;
	GameInput LastInput;
	ImageAsset* ImageAssets[16];
	SpriteSheetAsset* ShipObjectsSheet;
	int32 Frame;
	rnd_pcg_t RandomGen;
	// PhysWorld* Physics;
	GameWorld* World;
	// GameSystem* Systems[];
	EntityId PlayerEntity;
	flt32 Timer;
} GGame;

bool GameInitialize(const GameInitParams* params)
{
	LogLevel LoggingLevel = LogLevel_Info;
#ifndef _DEBUG
	LoggingLevel = LogLevel_Error;
#endif
	LoggingInitialize(LoggingLevel);
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
	GGame.Renderer = SDL_CreateRenderer(GGame.Window, NULL, SDL_RENDERER_ACCELERATED);
	SDL_SetRenderLogicalPresentation(
		GGame.Renderer, GameResWidth, GameResHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX, SDL_SCALEMODE_NEAREST);

	DebugInitialize(&(DebugConfig){
		.CanvasWidth = GameResWidth,
		.CanvasHeight = GameResHeight,
	});

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

	if (!GGame.World) {
		LogError("Shit");
		exit(1);
	}

	EntityId Entity0 = CreateEntity(GGame.World);
	EntityId Entity1 = CreateEntity(GGame.World);
	EntityId Entity2 = CreateEntity(GGame.World);

	GGame.PlayerEntity = Entity0;

	*AddComponent(TransformComponent, GGame.World, Entity0) = (TransformComponent){
		.Position = V2(64.0f, 128.0f),
		.Rotation = 0.25f,
	};
	AddComponent(VelocityComponent, GGame.World, Entity0);

	*AddComponent(SpriteComponent, GGame.World, Entity0) = (SpriteComponent){
		.SpriteId = SPRITE_ID(
			SpriteSheetId_ShipObjects, FindSpriteByName(GGame.ShipObjectsSheet->Data, GetStringId("green_01"))),
	};

	*AddComponent(ColliderComponent, GGame.World, Entity0) = (ColliderComponent){
		.Type = ColliderType_Polygon,
		.Group = Group_Friendly,
		.Polygon = PolygonCreateBox(V2(20.0f, 22.0f), V2(0, 0), 0.0f),
	};

	*AddComponent(TransformComponent, GGame.World, Entity1) = (TransformComponent){
		.Position = V2(256.0f, 128.0f),
		.Rotation = -0.25f,
	};

	*AddComponent(SpriteComponent, GGame.World, Entity1) = (SpriteComponent){
		.SpriteId =
			SPRITE_ID(SpriteSheetId_ShipObjects, FindSpriteByName(GGame.ShipObjectsSheet->Data, GetStringId("red_01"))),
	};

	*AddComponent(ColliderComponent, GGame.World, Entity1) = (ColliderComponent){
		.Type = ColliderType_Polygon,
		.Group = Group_Hostile,
		.Polygon = PolygonCreateBox(V2(20.0f, 20.0f), V2(0, 0), 0.0f),
	};

	*AddComponent(TransformComponent, GGame.World, Entity2) = (TransformComponent){0};

	LogInfo("Game Initialization Complete");

	return true;
}

static EntityId CreateProjectile(GameWorld* World, Vec2 Position, flt32 Rotation, flt32 Speed)
{
	EntityId Entity = CreateEntity(World);
	TransformComponent* T = AddComponent(TransformComponent, World, Entity);
	*T = (TransformComponent){
		.Position = Position,
		.Rotation = Rotation,
	};
	VelocityComponent* V = AddComponent(VelocityComponent, World, Entity);
	*V = (VelocityComponent){
		.AngularVelocity = 0.0f,
		.Velocity = Mul(R2(Rotation), Speed),
	};
	SpriteComponent* S = AddComponent(SpriteComponent, World, Entity);
	*S = (SpriteComponent){
		.SpriteId = SPRITE_ID(
			SpriteSheetId_ShipObjects, FindSpriteByName(GGame.ShipObjectsSheet->Data, GetStringId("projectile01-3"))),
		.Rotation = 0.25f,
	};

	*AddComponent(ColliderComponent, World, Entity) = (ColliderComponent){
		.Type = ColliderType_Circle,
		.Group = Group_Friendly,
		.Circle = {
			.Radius = 8.0f,
		}};

	*AddComponent(LifetimeComponent, World, Entity) = (LifetimeComponent){
		.SecondsRemaining = 2.0f,
	};
	//  GetComponent(TransformComponent, GGame.World, Entity);
	if (T->Position.Y < 10) {
		LogError("HELP");
		int p = 0;
	}
	return Entity;
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
	if (event->type == SDL_EVENT_KEY_DOWN && event->key.keysym.scancode == SDL_SCANCODE_F) {
		LogError("Error test");
	}
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

AABB ColliderCalcAABB(const ColliderComponent* Collider, Tform2 Transform)
{
	switch (Collider->Type) {
		case ColliderType_Circle: return CircleCalcAABB(&Collider->Circle, Transform);
		case ColliderType_Polygon: return PolygonCalcAABB(&Collider->Polygon, Transform);
		default:
			ASSERT(0 && "Unhandled case.");
			unreachable();
			break;
	}

	return (AABB){
		.MinBound = V2(KMaxFloat32, KMaxFloat32),
		.MaxBound = V2(-KMaxFloat32, -KMaxFloat32),
	};
}

void GameUpdate(const GameTime* gameTime)
{
	DebugNextFrame();

	int FramesPerSecond = (int)round(1.0 / gameTime->DeltaTime);
	DebugPrintf("FPS: %d, SIM: %0.3fms", FramesPerSecond, gameTime->SimTimeMS);
	DebugPrintf("Entities: %d", WorldEntityCount(GGame.World));

	{
		VelocityComponent* V = GetComponent(VelocityComponent, GGame.World, GGame.PlayerEntity);
		Vec2 MoveXY = InputXY(SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN);
		V->Velocity = Mul(Norm(MoveXY), 128.0f);
		if (GGame.Timer > 0.0f) GGame.Timer -= gameTime->DeltaTimeF;
		if (InputKey(SDL_SCANCODE_Z) && GGame.Timer <= 0.0f) {
			GGame.Timer += 0.17f;
			TransformComponent* T = GetComponent(TransformComponent, GGame.World, GGame.PlayerEntity);
			Vec2 SpawnPosition = Add(T->Position, V2(16.0f, 0.0));
			flt32 SpawnRotation = T->Rotation - 0.25f;
			for (int i = -1; i <= 1; i += 1) {
				CreateProjectile(GGame.World, SpawnPosition, SpawnRotation + (i * 0.05f), 256.0f);
			}
		}
	}

	{
		EntityId* Query = WorldQueryEntities(GGame.World, REQUIRED(Transform, Velocity), REJECTED());
		for (EntityId* Iter = Query; Iter != arrend(Query); Iter++) {
			EntityId Entity = *Iter;
			TransformComponent* T = GetComponent(TransformComponent, GGame.World, Entity);
			VelocityComponent* V = GetComponent(VelocityComponent, GGame.World, Entity);

			T->Position = Add(T->Position, Mul(V->Velocity, gameTime->DeltaTimeF));

			T->Rotation += V->AngularVelocity * gameTime->DeltaTimeF;
		}
		WorldQueryFree(Query);
	}

	{
		EntityId* Query = WorldQueryEntities(GGame.World, REQUIRED(Transform, Collider), REJECTED());
		for (EntityId* Iter0 = Query; Iter0 != arrend(Query); Iter0++) {
			EntityId Entity0 = *Iter0;
			ColliderComponent* Collider0 = GetComponent(ColliderComponent, GGame.World, Entity0);
			TransformComponent* Transform0 = GetComponent(TransformComponent, GGame.World, Entity0);
			AABB Bounds0 = ColliderCalcAABB(Collider0, T2(Transform0->Position, R2(Transform0->Rotation)));
			for (EntityId* Iter1 = Query; Iter1 != arrend(Query); Iter1++) {
				EntityId Entity1 = *Iter1;
				if (!ENTITY_ID_EQ(Entity0, Entity1)) {
					ColliderComponent* Collider1 = GetComponent(ColliderComponent, GGame.World, Entity1);
					if (Collider0->Group != Collider1->Group) {
						TransformComponent* Transform1 = GetComponent(TransformComponent, GGame.World, Entity1);
						AABB Bounds1 = ColliderCalcAABB(Collider1, T2(Transform1->Position, R2(Transform1->Rotation)));
						if (AABBTestOverlap(Bounds0, Bounds1)) {

							LogInfo("Intersection %d %d", Entity0.RawValue, Entity1.RawValue);
						}
					}
				}
			}
		}
		WorldQueryFree(Query);
	}

	{
		EntityId* ToDelete = NULL;
		arrsetcap(ToDelete, WorldEntityCount(GGame.World));
		EntityId* Query = WorldQueryEntities(GGame.World, REQUIRED(Lifetime), REJECTED());
		for (EntityId* Iter = Query; Iter != arrend(Query); Iter++) {
			EntityId Entity = *Iter;
			LifetimeComponent* L = GetComponent(LifetimeComponent, GGame.World, Entity);
			if (L->SecondsRemaining > 0.0f) {
				L->SecondsRemaining -= gameTime->DeltaTimeF;
				if (L->SecondsRemaining <= 0.0f) {
					arrput(ToDelete, Entity);
				}
			}
		}
		WorldQueryFree(Query);

		for (int32 Index = 0; Index < arrlen(ToDelete); Index++) {
			DestroyEntity(GGame.World, ToDelete[Index]);
		}
		arrfree(ToDelete);
	}

	GGame.Frame++;
}

void GameRender(const GameTime* gameTime)
{
	SDL_SetRenderDrawColor(GGame.Renderer, 0, 0, 0, 255);
	SDL_RenderClear(GGame.Renderer);

	int RenderWidth, RenderHeight;
	SDL_GetRenderLogicalPresentation(GGame.Renderer, &RenderWidth, &RenderHeight, NULL, NULL);

	SDL_FRect BgRect = {0, 0, RenderWidth, RenderHeight};
	SDL_SetRenderDrawColor(GGame.Renderer, 0x12, 0x20, 0x20, 255);
	// SDL_SetRenderDrawColor(GGame.Renderer, 0xCC, 0xCC, 0xCC, 255);
	// SDL_SetRenderDrawColor(GGame.Renderer, 0, 0, 0, 255);
	SDL_RenderFillRect(GGame.Renderer, &BgRect);

	// Background nebula
	DrawSprite(&(SpriteDraw){
		.SpriteId = SPRITE_ID(SpriteSheetId_BGObjects0, 44),
		.Position = {372.0f, 128.0f},
		.SpriteTiles = {4, 4},
	});

	EntitySignature Sig = SIGNATURE(Transform, Sprite);

	{
		EntityId* Query = WorldQueryEntities(GGame.World, REQUIRED(Transform, Sprite), REJECTED());
		for (EntityId *Iter = Query, *Last = arrend(Query); Iter != Last; Iter++) {
			EntityId Entity = *Iter;
			TransformComponent* T = GetComponent(TransformComponent, GGame.World, Entity);
			SpriteComponent* S = GetComponent(SpriteComponent, GGame.World, Entity);
			DrawSprite(&(SpriteDraw){
				.SpriteId = S->SpriteId,
				.Position = Add(T->Position, S->Offset),
				.Rotation = T->Rotation + S->Rotation,
			});
		}
		WorldQueryFree(Query);
	}

	{
		EntityId* Query = WorldQueryEntities(GGame.World, REQUIRED(Transform, Collider), REJECTED());
		for (EntityId* Iter = Query; Iter != arrend(Query); Iter++) {
			EntityId Entity = *Iter;
			TransformComponent* T = GetComponent(TransformComponent, GGame.World, Entity);
			ColliderComponent* S = GetComponent(ColliderComponent, GGame.World, Entity);
			Tform2 Transform = T2(T->Position, R2(T->Rotation));
			switch (S->Type) {
				case ColliderType_Circle:
					DrawCircle(TransformV2(Transform, S->Circle.Center), S->Circle.Radius, 0xFFFFFF00);
					break;
				case ColliderType_Polygon:
					DrawPolygon(T->Position, R2(T->Rotation), S->Polygon.Vertices, S->Polygon.VertexCount, 0xFFFFFF00);
					break;
				default: unreachable(); break;
			}
		}
		WorldQueryFree(Query);
	}

	DrawRender();

	DebugDraw(GGame.Renderer);
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
