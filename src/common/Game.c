#include "Game.h"

#include "ini.h"
#include <SDL3/SDL.h>
#include <stb_ds.h>
#include <stdio.h>
#include <stdlib.h>

#include "Algorithm.h"
#include "AssetTypes.h"
#include "Debug.h"
#include "Draw.h"
#include "FluidSim.h"
#include "FrameAllocator.h"
#include "GameWorld.h"
#include "JsonHelpers.h"
#include "Log.h"
#include "Math2D.h"
#include "Physics.h"
#include "Random.h"
#include "RenderUtil.h"
#include "SdlEventHandler.h"
#include "SpriteDatabase.h"
#include "StringId.h"

enum {
	Group_Friendly,
	Group_Hostile,
};

enum { KSpriteAnimationMaxFrames = 64 };

typedef struct SpriteAnimationFrameData {
	SpriteId Sprite;
	Vec2 PositionOffset;
	float32 RotationOffset;
} SpriteAnimationFrameData;

typedef struct SpriteAnimationData {
	SpriteAnimationFrameData Frames[KSpriteAnimationMaxFrames];
	int32 FrameCount;
	float32 SecondsPerFrame;
} SpriteAnimationData;

static EntityId CreateProjectile(GameWorld* World, Vec2 Position, float32 Rotation, float32 Speed);
static EntityId CreatePlayerShip(GameWorld* World, Vec2 Position);
static EntityId CreateEnemy(GameWorld* World, Vec2 Position);

void ApplyPlayerControl(GameWorld* World, const GameTime* Time, EntityId Entity);
void MovementSystemUpdate(GameWorld* World, const GameTime* Time);
void DamageSystemUpdate(GameWorld* World, const GameTime* Time);
void LifetimeSystemUpdate(GameWorld* World, const GameTime* Time);
void BehaviorSystemUpdate(GameWorld* World, const GameTime* Time);
void SpriteSystemRender(GameWorld* World);
void ColliderSystemDebugRender(GameWorld* World);

void ResetFluidSim(void);
void ReconfigureFluidSim(void);

bool GameHandleSdlEvent(const SDL_Event* Event, void* Context);

struct {
	bool IsRunning;
	SDL_Window* Window;
	SDL_Renderer* Renderer;
	GameInput Input;
	GameInput LastInput;
	int32 Frame;
	rnd_pcg_t RandomGen;
	SpriteSheetId ShipObjectsSpriteSheetHandle;
	SpriteSheetId BackgroundObjectsSpriteSheetHandle;
	SpriteSheetId FlaresSpriteSheetHandle;
	// PhysWorld* Physics;
	GameWorld* World;
	EntityId PlayerEntity;
	EntityId LevelEntity;
	float32 Timer;
	float32 SecondTimer;
	int32 LastFPS;
	int32 FramesThisSecond;
	bool DebugDrawEnabled;
	FluidSim* FluidSim;
} GGame;

SpriteAnimationData GBossIdleAnimationData;
EntityId GBossEntity;

bool GameInitialize(const GameInitParams* params)
{
	AddSdlEventHandler(GameHandleSdlEvent, NULL);

	FrameAllocatorInitialize(KILOBYTES(640));

	RandomSetSeed((uint32)SDL_GetPerformanceCounter());
	uint32 RandomSeed = (uint32)SDL_GetPerformanceCounter();
	rnd_pcg_seed(&GGame.RandomGen, RandomSeed);
	uint64 HashtableSeed = (uint64)rnd_pcg_next(&GGame.RandomGen) | (((uint64)rnd_pcg_next(&GGame.RandomGen)) << 32);
	stbds_rand_seed(HashtableSeed);

	StringIdPoolsInitialize();

	StringId RenderDriverNameId = GetStringId("vulkan");
	struct json_object_s* JsonConfig = JsonLoadFileAsObject("config.json");
	if (JsonConfig) {
		StringId RequestedRenderDriver = JsonGetStringId(JsonConfig, "render_driver", KInvalidStringId);
		if (StringIdIsValid(RequestedRenderDriver)) {
			RenderDriverNameId = RequestedRenderDriver;
		}
	}

	const char* RenderDriverName = StringIdCStr(RenderDriverNameId);
	const char* SelectedRenderDriver = SelectRenderDriver(RenderDriverName);

	LogInfo("Creating renderer with '%s' driver.", SelectedRenderDriver);

	GGame.IsRunning = true;
#ifdef _DEBUG
	GGame.DebugDrawEnabled = true;
#endif
	GGame.Window = params->Window;
	GGame.Renderer = SDL_CreateRenderer(GGame.Window, SelectedRenderDriver);
	int WindowWidth, WindowHeight;
	SDL_GetWindowSizeInPixels(GGame.Window, &WindowWidth, &WindowHeight);
	SDL_SetRenderLogicalPresentation(GGame.Renderer, 640, 360, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	SDL_SetRenderDrawBlendMode(GGame.Renderer, SDL_BLENDMODE_BLEND);

	DebugInitialize(&(DebugConfig){
		.CanvasWidth = WindowWidth,
		.CanvasHeight = WindowHeight,
		.Renderer = GGame.Renderer,
		.BackgroundColor = 0xAFF0207F,
		.ForegroundColor = 0xFFF0CF7F,
		.Margin = 2,
	});

	AssetsInitialize(&(AssetsConfig){.TypeConfigs = {
										 [AssetType_Image] =
											 {.LoadAssetData = (LoadAssetDataFunc)LoadImageData,
											  .UnloadAssetData = (UnloadAssetDataFunc)UnloadImageData,
											  .Size = sizeof(ImageData)},
										 [AssetType_SpriteSheetData] =
											 {.LoadAssetData = (LoadAssetDataFunc)LoadSpriteSheetData,
											  .UnloadAssetData = (UnloadAssetDataFunc)UnloadSpriteSheetData,
											  .Size = sizeof(SpriteSheetData)},
									 }});

	ImageAsset* ShipObjectsSpriteSheetImageAsset =
		(ImageAsset*)LoadAsset(AssetType_Image, "assets/spritesheets/ship_objects/ship_objects.png");
	ImageAsset* BackgroundObjectsSpriteSheetImageAsset =
		(ImageAsset*)LoadAsset(AssetType_Image, "assets/CelestialObjects.png");
	ImageAsset* FlaresSpriteSheetImageAsset = (ImageAsset*)LoadAsset(AssetType_Image, "assets/flares.png");
	ImageAsset* BigFlaresSpriteSheetImageAsset = (ImageAsset*)LoadAsset(AssetType_Image, "assets/bigflare.png");
	ImageAsset* SmallFlaresSpriteSheetImageAsset = (ImageAsset*)LoadAsset(AssetType_Image, "assets/smallflare.png");

	SpriteSheetAsset* ShipObjectsSpriteSheetDataAsset =
		(SpriteSheetAsset*)LoadAsset(AssetType_SpriteSheetData, "assets/spritesheets/ship_objects/ship_objects.json");

	SpriteDatabaseInitialize(GGame.Renderer);
	GGame.ShipObjectsSpriteSheetHandle = SpriteDatabaseCreateFrameDataSpriteSheet(
		GetStringId("ShipObjects"),
		ShipObjectsSpriteSheetImageAsset,
		ShipObjectsSpriteSheetDataAsset);
	GGame.BackgroundObjectsSpriteSheetHandle = SpriteDatabaseCreateGridSpriteSheet(
		GetStringId("BackgroundObjects"),
		BackgroundObjectsSpriteSheetImageAsset,
		32,
		32);

	GGame.FlaresSpriteSheetHandle =
		SpriteDatabaseCreateGridSpriteSheet(GetStringId("Flares"), FlaresSpriteSheetImageAsset, 64, 64);
	SpriteDatabaseCreateGridSpriteSheet(GetStringId("BigFlare"), BigFlaresSpriteSheetImageAsset, 512, 512);
	SpriteDatabaseCreateGridSpriteSheet(GetStringId("SmallFlare"), SmallFlaresSpriteSheetImageAsset, 128, 128);

	DrawInitialize(&(DrawConfig){
		.Renderer = GGame.Renderer,
	});

	GGame.World = CreateGameWorld();

	if (!GGame.World) {
		LogError("Shit");
		exit(1);
	}

	GGame.LevelEntity = CreateEntity(GGame.World);
	*AddComponent(RenderTintComponent, GGame.World, GGame.LevelEntity) = (RenderTintComponent){
		.TintColor = V4(0.0f, 0.0f, 0.1f, 1.0f),
	};

	GGame.PlayerEntity = CreatePlayerShip(GGame.World, V2(64, 128));
	CreateEnemy(GGame.World, V2(256, 128));

	{
		EntityId BackgroundEntity = CreateEntity(GGame.World);
		*AddComponent(TransformComponent, GGame.World, BackgroundEntity) = (TransformComponent){
			.Position = V2(372, 128),
		};
		*AddComponent(SpriteComponent, GGame.World, BackgroundEntity) = (SpriteComponent){
			.SpriteId = SpriteSheetFindSpriteByIndex(GGame.BackgroundObjectsSpriteSheetHandle, 44),
		};
		*AddComponent(SpriteTilesComponent, GGame.World, BackgroundEntity) = (SpriteTilesComponent){
			.Tiles = {4, 4},
		};
		*AddComponent(RenderLayerComponent, GGame.World, BackgroundEntity) = (RenderLayerComponent){
			.Layer = -1000,
		};
	}

	GBossIdleAnimationData = (SpriteAnimationData){
		.Frames =
			{
				{.Sprite = SpriteFindByName("boss-01-1")}, {.Sprite = SpriteFindByName("boss-01-2")},
				{.Sprite = SpriteFindByName("boss-01-3")}, {.Sprite = SpriteFindByName("boss-01-4")},
				{.Sprite = SpriteFindByName("boss-01-5")}, {.Sprite = SpriteFindByName("boss-01-6")},
				{.Sprite = SpriteFindByName("boss-01-7")}, {.Sprite = SpriteFindByName("boss-01-8")},
				{.Sprite = SpriteFindByName("boss-01-8")}, {.Sprite = SpriteFindByName("boss-01-8")},
				{.Sprite = SpriteFindByName("boss-01-8")}, {.Sprite = SpriteFindByName("boss-01-8")},
				{.Sprite = SpriteFindByName("boss-01-7")}, {.Sprite = SpriteFindByName("boss-01-6")},
				{.Sprite = SpriteFindByName("boss-01-5")}, {.Sprite = SpriteFindByName("boss-01-4")},
				{.Sprite = SpriteFindByName("boss-01-3")}, {.Sprite = SpriteFindByName("boss-01-2")},
				{.Sprite = SpriteFindByName("boss-01-1")},
			},
		.FrameCount = 19,
		.SecondsPerFrame = 1.0f / 6.0f,
	};

	GBossEntity = CreateEntity(GGame.World);
	AddComponent(TransformComponent, GGame.World, GBossEntity)->Position = V2(600, 100);
	AddComponent(SpriteComponent, GGame.World, GBossEntity);
	AddComponent(TimerComponent, GGame.World, GBossEntity);

	LogInfo("Game Initialization Complete");

	ResetFluidSim();
	return true;
}

static EntityId CreateProjectile(GameWorld* World, Vec2 Position, float32 Rotation, float32 Speed)
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
		.SpriteId = SpriteFindByName("projectile01-3"),
		.Rotation = 0.25f,
	};

	*AddComponent(ColliderComponent, World, Entity) = (ColliderComponent){.Type = ColliderType_Circle,
																		  .Group = Group_Friendly,
																		  .Circle = {
																			  .Radius = 8.0f,
																		  }};

	*AddComponent(LifetimeComponent, World, Entity) = (LifetimeComponent){
		.SecondsRemaining = 2.0f,
	};

	*AddComponent(DamageSourceComponent, World, Entity) = (DamageSourceComponent){
		.DamageAmount = 1.0f,
	};

	return Entity;
}

static EntityId CreatePlayerShip(GameWorld* World, Vec2 Position)
{
	LogInfo(__FUNCTION__);

	EntityId Entity = CreateEntity(World);

	NameEntity(World, Entity, "PlayerTank");
	*AddComponent(TransformComponent, World, Entity) = (TransformComponent){
		.Position = Position,
		.Rotation = 0.25f,
	};
	AddComponent(VelocityComponent, World, Entity);
	*AddComponent(SpriteComponent, World, Entity) = (SpriteComponent){
		.SpriteId = SpriteFindByName("tankbase_02"),
	};
	*AddComponent(ColliderComponent, World, Entity) = (ColliderComponent){
		.Type = ColliderType_Polygon,
		.Group = Group_Friendly,
		.Polygon = PolygonCreateBox(V2(20.0f, 22.0f), V2(0, 0), 0.0f),
	};

	EntityId TurretEntity = CreateEntity(World);
	NameEntity(World, TurretEntity, "PlayerTankTurret");
	AddComponent(TransformComponent, World, TurretEntity);
	AddComponent(LocalTransformComponent, World, TurretEntity);
	AddComponent(ChildOfComponent, World, TurretEntity)->Parent = Entity;
	AddComponent(SpriteComponent, World, TurretEntity)->SpriteId = SpriteFindByName("tankcannon-01A");
	AddComponent(BehaviorComponent, World, TurretEntity);

	return Entity;
}

static EntityId CreateEnemy(GameWorld* World, Vec2 Position)
{
	EntityId Entity = CreateEntity(World);

	*AddComponent(TransformComponent, World, Entity) = (TransformComponent){
		.Position = Position,
		.Rotation = -0.25f,
	};

	*AddComponent(SpriteComponent, World, Entity) = (SpriteComponent){
		.SpriteId = SpriteFindByName("red_01"),
	};

	*AddComponent(ColliderComponent, World, Entity) = (ColliderComponent){
		.Type = ColliderType_Polygon,
		.Group = Group_Hostile,
		.Polygon = PolygonCreateBox(V2(20.0f, 20.0f), V2(0, 0), 0.0f),
	};

	*AddComponent(DamageReceiverComponent, World, Entity) = (DamageReceiverComponent){};
	*AddComponent(DurabilityComponent, World, Entity) = (DurabilityComponent){.CurrentDurability = 10.0f};

	return Entity;
}

void GameShutdown(void)
{
	LogInfo(__FUNCTION__);

	DestroyFluidSim(GGame.FluidSim);

	ClearSdlEventHandlers();
	SpriteDatabaseShutdown();
	DrawShutdown();
	AssetsShutdown();
	DebugShutdown();

	LogInfo("Destroying Renderer");
	SDL_DestroyRenderer(GGame.Renderer);

	// DestroyGameWorld(GGame.World);
	StringIdPoolsShutdown();
	FrameAllocatorShutdown();

	LoggingShutdown();
}

void GameSendInput(const GameInput* input)
{
	memcpy(&GGame.LastInput, &GGame.Input, sizeof(GameInput));
	memcpy(&GGame.Input, input, sizeof(GameInput));
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

static float32 InputAxis(int ScancodeNeg, int ScancodePos)
{
	float32 Axis = 0.0f;
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
	FrameAllocatorNextFrame();
	DebugNextFrame();

	ApplyPlayerControl(GGame.World, gameTime, GGame.PlayerEntity);
	MovementSystemUpdate(GGame.World, gameTime);

	{
		EntityId* Query = WorldQueryEntities(GGame.World, REQUIRED(Transform, LocalTransform, ChildOf), REJECTED());
		for (EntityId* Iter = QueryBegin(Query); Iter != QueryEnd(Query); Iter++) {
			// todo: very dumb and bad just getting absolute basic case working
			// Tform2 ParentTransform;
			TransformComponent* Transform = GetComponent(TransformComponent, GGame.World, *Iter);
			LocalTransformComponent* LocalTransform = GetComponent(LocalTransformComponent, GGame.World, *Iter);
			ChildOfComponent* ChildOf = GetComponent(ChildOfComponent, GGame.World, *Iter);
			Tform2 ParentT2 = (Tform2){0};
			float32 ParentRotation = 0.0f;
			if (ChildOf->Parent.RawValue != 0) {
				TransformComponent* ParentTransform = GetComponent(TransformComponent, GGame.World, ChildOf->Parent);
				ParentT2 = T2Component(ParentTransform);
				ParentRotation = ParentTransform->Rotation;
			}
			Transform->Position = TransformV2(ParentT2, LocalTransform->LocalPosition);
			Transform->Rotation = ParentRotation + LocalTransform->LocalRotation;
		}
		QueryFree(Query);
	}

	DamageSystemUpdate(GGame.World, gameTime);
	LifetimeSystemUpdate(GGame.World, gameTime);

	FluidSimUpdate(GGame.FluidSim, 1.0f / 120);

	GGame.FramesThisSecond++;
	GGame.SecondTimer += gameTime->DeltaTimeF;
	if (GGame.SecondTimer >= 1.0f) {
		GGame.SecondTimer -= 1.0f;
		GGame.LastFPS = GGame.FramesThisSecond;
		GGame.FramesThisSecond = 0;
	}
	DebugPrintf("FPS: %d, SIM: %0.3fms, DRAW: %06.3fms", GGame.LastFPS, gameTime->SimTimeMS, gameTime->RenderTimeMS);
	// DebugPrintf("Entities: %d", WorldEntityCount(GGame.World));
	static bool ShowComponentCounts = false;
	if (ShowComponentCounts) {
		int32 ComponentCounts[ComponentType_Count];
		WorldComponentCounts(GGame.World, ComponentCounts, ComponentType_Count);
		for (int32 Index = 0; Index < ComponentType_Count; Index++) {
			DebugPrintf(" %s: %d", ComponentTypeName(Index), ComponentCounts[Index]);
		}
	}

	GGame.Frame++;
}

void GameRender(const GameTime* gameTime)
{
	SDL_SetRenderDrawColor(GGame.Renderer, 0, 0, 0, 255);
	SDL_RenderClear(GGame.Renderer);

	SDL_Color BackgroundColor = {};
	RenderTintComponent* BackgroundTint = TryGetComponent(RenderTintComponent, GGame.World, GGame.LevelEntity);
	if (BackgroundTint != NULL) {
		ColorU8 Color = ColorU8FromVec4(BackgroundTint->TintColor);
		BackgroundColor = *(SDL_Color*)&Color;
	}

	// SpriteSystemRender(GGame.World);
	// ColliderSystemDebugRender(GGame.World);
	FluidSimRender(GGame.FluidSim, GGame.Renderer);

	SDL_SetRenderDrawColor(GGame.Renderer, BackgroundColor.r, BackgroundColor.g, BackgroundColor.b, BackgroundColor.a);
	SDL_RenderClear(GGame.Renderer);

	// PhysicsDebugDraw(GGame.Renderer);
	DrawRender();

	Vec2 MousePos;
	SDL_GetMouseState(&MousePos.X, &MousePos.Y);
	// LagrangianFluidSimDebugRender((LagrangianFluidSim*)GGame.FluidSim, GGame.Renderer, MousePos);
	
	if (GGame.DebugDrawEnabled) {
		DebugDraw(GGame.Renderer);
	}


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

bool GameHandleSdlEvent(const SDL_Event* Event, void* Context)
{
	bool Handled = false;

	switch (Event->type) {
		case SDL_EVENT_KEY_DOWN:
			switch (Event->key.scancode) {
				case SDL_SCANCODE_F1:
					GGame.DebugDrawEnabled = !GGame.DebugDrawEnabled;
					Handled = true;
					break;
				case SDL_SCANCODE_F:
					if (EntityIdIsValid(GGame.World, GGame.PlayerEntity)) {
						DestroyEntity(GGame.World, GGame.PlayerEntity);
						LogError("DELETED!");
					}
					Handled = true;
					break;
				case SDL_SCANCODE_R:
					if ((Event->key.mod & SDL_KMOD_SHIFT) != 0) {
						ResetFluidSim();
					} else {
						ReconfigureFluidSim();
					}
					break;
				default: break;
			}
			break;
		default: break;
	}

	return Handled;
}

void ApplyPlayerControl(GameWorld* World, const GameTime* Time, EntityId Entity)
{
	if (EntityIdIsValid(World, GGame.PlayerEntity)) {
		VelocityComponent* V = GetComponent(VelocityComponent, World, GGame.PlayerEntity);
		Vec2 MoveXY = InputXY(SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN);
		V->Velocity = Mul(Norm(MoveXY), 84.0f);
		// V->AngularVelocity = 0.1f;
		if (GGame.Timer > 0.0f) GGame.Timer -= Time->DeltaTimeF;
		if (InputKey(SDL_SCANCODE_Z) && GGame.Timer <= 0.0f) {
			GGame.Timer += 0.18f;
			TransformComponent* T = GetComponent(TransformComponent, World, GGame.PlayerEntity);
			Vec2 SpawnPosition = Add(T->Position, V2(16.0f, 0.0));
			float32 SpawnRotation = T->Rotation - 0.25f;
			for (int i = -5; i <= 5; i += 1) {
				CreateProjectile(World, SpawnPosition, SpawnRotation + (i * 0.015f), 256.0f);
			}
		}
	}
}

void MovementSystemUpdate(GameWorld* World, const GameTime* Time)
{
	EntityId* Query = WorldQueryEntities(World, REQUIRED(Transform, Velocity), REJECTED());
	for (EntityId* Iter = QueryBegin(Query); Iter != QueryEnd(Query); Iter++) {
		EntityId Entity = *Iter;
		TransformComponent* T = GetComponent(TransformComponent, World, Entity);
		VelocityComponent* V = GetComponent(VelocityComponent, World, Entity);

		T->Position = Add(T->Position, Mul(V->Velocity, Time->DeltaTimeF));
		T->Rotation += V->AngularVelocity * Time->DeltaTimeF;
	}
	QueryFree(Query);
}

struct DamageEvent {
	EntityId DamageReceiver;
	EntityId DamageSource;
};

static int32 DamageEventCompare(struct DamageEvent A, struct DamageEvent B)
{
	return ENTITY_ID_EQ(A.DamageReceiver, B.DamageReceiver) ? EntityIdCompare(A.DamageSource, B.DamageSource)
															: EntityIdCompare(A.DamageReceiver, B.DamageReceiver);
}

static int DamageEventCompareVoid(const void* A, const void* B)
{
	return DamageEventCompare(*(const struct DamageEvent*)A, *(const struct DamageEvent*)B);
}

void DamageSystemUpdate(GameWorld* World, const GameTime* Time)
{
	struct DamageEvent* DamageEvents = NULL;
	arrsetcap(DamageEvents, 256);

	EntityId* Query = WorldQueryEntities(World, REQUIRED(Transform, Collider), REJECTED());
	for (EntityId* Iter0 = QueryBegin(Query); Iter0 != QueryEnd(Query); Iter0++) {
		EntityId Entity0 = *Iter0;
		ColliderComponent* Collider0 = GetComponent(ColliderComponent, World, Entity0);
		Tform2 Transform0 = T2Component(GetComponent(TransformComponent, World, Entity0));
		AABB Bounds0 = ColliderCalcAABB(Collider0, Transform0);
		for (EntityId* Iter1 = Iter0 + 1; Iter1 != QueryEnd(Query); Iter1++) {
			EntityId Entity1 = *Iter1;
			if (!ENTITY_ID_EQ(Entity0, Entity1)) {
				ColliderComponent* Collider1 = GetComponent(ColliderComponent, World, Entity1);
				if (Collider0->Group != Collider1->Group) {
					Tform2 Transform1 = T2Component(GetComponent(TransformComponent, World, Entity1));
					AABB Bounds1 = ColliderCalcAABB(Collider1, Transform1);
					if (AABBTestOverlap(Bounds0, Bounds1)) {
						if (ColliderIntersectsCollider(Collider0, Transform0, Collider1, Transform1)) {
							if (HasComponent(DamageReceiverComponent, World, Entity0) &&
								HasComponent(DamageSourceComponent, World, Entity1))
							{
								arrput(
									DamageEvents,
									((struct DamageEvent){.DamageReceiver = Entity0, .DamageSource = Entity1}));
							}
							if (HasComponent(DamageReceiverComponent, World, Entity1) &&
								HasComponent(DamageSourceComponent, World, Entity0))
							{
								arrput(
									DamageEvents,
									((struct DamageEvent){.DamageReceiver = Entity1, .DamageSource = Entity0}));
							}
						}
					}
				}
			}
		}
	}
	QueryFree(Query);

	SDL_qsort(DamageEvents, arrlenu(DamageEvents), sizeof(*DamageEvents), DamageEventCompareVoid);

	for (int32 Index = 0; Index < arrlen(DamageEvents); Index++) {
		EntityId SourceEntity = DamageEvents[Index].DamageSource;
		EntityId ReceiverEntity = DamageEvents[Index].DamageReceiver;
		DamageSourceComponent* Source = GetComponent(DamageSourceComponent, World, SourceEntity);
		DamageReceiverComponent* Receiver = GetComponent(DamageReceiverComponent, World, ReceiverEntity);

		Receiver->DamageAccumulator += Source->DamageAmount;
		GetOrAddComponent(LifetimeComponent, World, SourceEntity)->SecondsRemaining = 0.0f;

		LogInfo("Damage Event: %d -> %d", DamageEvents[Index].DamageSource, DamageEvents[Index].DamageReceiver);
	}

	arrfree(DamageEvents);

	{
		EntityId* Query = WorldQueryEntities(World, REQUIRED(DamageReceiver, Durability), REJECTED());
		for (EntityId* Iter = QueryBegin(Query); Iter != QueryEnd(Query); Iter++) {
			DamageReceiverComponent* Receiver = GetComponent(DamageReceiverComponent, World, *Iter);
			DurabilityComponent* Durability = GetComponent(DurabilityComponent, World, *Iter);
			Durability->CurrentDurability -= Receiver->DamageAccumulator;
			Receiver->DamageAccumulator = 0.0f;
			if (Durability->CurrentDurability <= 0.0f) {
				DestroyEntity(World, *Iter);
			}
		}
		QueryFree(Query);
	}
}

void LifetimeSystemUpdate(GameWorld* World, const GameTime* Time)
{
	EntityId* Query = WorldQueryEntities(World, REQUIRED(Lifetime), REJECTED());
	for (EntityId* Iter = QueryBegin(Query); Iter != QueryEnd(Query); Iter++) {
		EntityId Entity = *Iter;
		LifetimeComponent* L = GetComponent(LifetimeComponent, World, Entity);
		if (L->SecondsRemaining >= 0.0f) {
			L->SecondsRemaining -= Time->DeltaTimeF;
			if (L->SecondsRemaining <= 0.0f) {
				DestroyEntity(World, *Iter);
			}
		}
	}
	QueryFree(Query);
}

void SpriteSystemRender(GameWorld* World)
{
	EntityId* Query = WorldQueryEntities(World, REQUIRED(Transform, Sprite), REJECTED());
	for (EntityId *Iter = QueryBegin(Query), *Last = QueryEnd(Query); Iter != Last; Iter++) {
		EntityId Entity = *Iter;
		TransformComponent* T = GetComponent(TransformComponent, World, Entity);
		SpriteComponent* S = GetComponent(SpriteComponent, World, Entity);
		SpriteTilesComponent* Tiles = TryGetComponent(SpriteTilesComponent, World, Entity);
		RenderLayerComponent* Layer = TryGetComponent(RenderLayerComponent, World, Entity);
		RenderTintComponent* Tint = TryGetComponent(RenderTintComponent, World, Entity);
		DrawSprite(&(SpriteDraw){
			.SpriteId = S->SpriteId,
			.Position = Add(T->Position, S->Offset),
			.Rotation = T->Rotation + S->Rotation,
			.SpriteTiles = (Tiles != NULL) ? Tiles->Tiles : (Point){1, 1},
			.Layer = (Layer != NULL) ? Layer->Layer : 0,
			.UseTint = (Tint != NULL),
			.TintColor = (Tint != NULL) ? ColorU8FromVec4(Tint->TintColor) : (ColorU8){0},
		});
	}
	QueryFree(Query);
}

void ColliderSystemDebugRender(GameWorld* World)
{
	EntityId* Query = WorldQueryEntities(World, REQUIRED(Transform, Collider), REJECTED());
	for (EntityId* Iter = QueryBegin(Query); Iter != QueryEnd(Query); Iter++) {
		EntityId Entity = *Iter;
		TransformComponent* T = GetComponent(TransformComponent, World, Entity);
		ColliderComponent* S = GetComponent(ColliderComponent, World, Entity);
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
	QueryFree(Query);
}

void ResetFluidSim(void)
{
	if (GGame.FluidSim != NULL) {
		DestroyFluidSim(GGame.FluidSim);
		GGame.FluidSim = NULL;
	}

	LagrangianFluidSimConfig Config;

	struct json_value_s* ConfigJson = JsonLoadFile("assets/data/fluid_sim_config.json");
	if (JsonParseLagrangianFluidSimConfig(ConfigJson, &Config)) {
		GGame.FluidSim = CreateLagrangianFluidSim(&Config);
	}
}

void ReconfigureFluidSim(void)
{
	LagrangianFluidSimConfig Config;

	struct json_value_s* ConfigJson = JsonLoadFile("assets/data/fluid_sim_config.json");
	if (JsonParseLagrangianFluidSimConfig(ConfigJson, &Config)) {
		FluidSimReconfig(GGame.FluidSim, (FluidSimConfig*)&Config);
	}
}