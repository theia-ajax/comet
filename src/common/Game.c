#include "Game.h"

#include <SDL3/SDL.h>
#include <stb_ds.h>
#include <stdlib.h>

#include "AssetTypes.h"
#include "Debug.h"
#include "Draw.h"
#include "FrameAllocator.h"
#include "GameWorld.h"
#include "Log.h"
#include "Math2D.h"
#include "Physics.h"
#include "Random.h"
#include "SpriteDatabase.h"
#include "StringId.h"
#include "Util.h"

enum {
	Group_Friendly,
	Group_Hostile,
};

enum SpriteSheets {
	SpriteSheets_Default,
	SpriteSheets_ShipObjects,
	SpriteSheets_BackgroundObjects,
	SpriteSheets_Count,
};

static EntityId CreateProjectile(GameWorld* World, Vec2 Position, flt32 Rotation, flt32 Speed);
static EntityId CreatePlayerShip(GameWorld* World, Vec2 Position);
static EntityId CreateEnemy(GameWorld* World, Vec2 Position);

void ApplyPlayerControl(GameWorld* World, const GameTime* Time, EntityId Entity);
void MovementSystemUpdate(GameWorld* World, const GameTime* Time);
void DamageSystemUpdate(GameWorld* World, const GameTime* Time);
void LifetimeSystemUpdate(GameWorld* World, const GameTime* Time);
void SpriteSystemRender(GameWorld* World);
void ColliderSystemDebugRender(GameWorld* World);

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
	// PhysWorld* Physics;
	GameWorld* World;
	EntityId PlayerEntity;
	EntityId PlayerAnchorEntity;
	EntityId LevelEntity;
	flt32 Timer;
	flt32 SecondTimer;
	int32 LastFPS;
	int32 FramesThisSecond;
} GGame;

bool GameInitialize(const GameInitParams* params)
{
#ifdef _DEBUG
	LogLevel LoggingLevel = LogLevel_Info;
#else
	LogLevel LoggingLevel = LogLevel_Warning;
#endif

	LoggingInitialize(LoggingLevel);
	LogInfo(__FUNCTION__);

	FrameAllocatorInitialize(KILOBYTES(640));

	uint32 RandomSeed = (uint32)SDL_GetPerformanceCounter();
	rnd_pcg_seed(&GGame.RandomGen, RandomSeed);
	uint64 HashtableSeed = (uint64)rnd_pcg_next(&GGame.RandomGen) | (((uint64)rnd_pcg_next(&GGame.RandomGen)) << 32);
	stbds_rand_seed(HashtableSeed);

	StringIdPoolsInitialize();

	int32 GameResWidth = 640;
	int32 GameResHeight = 360;

	LogInfo("Creating Renderer");
	GGame.IsRunning = true;
	GGame.Window = params->Window;
	GGame.Renderer = SDL_CreateRenderer(GGame.Window, NULL, SDL_RENDERER_ACCELERATED);
	SDL_SetRenderLogicalPresentation(
		GGame.Renderer,
		GameResWidth,
		GameResHeight,
		SDL_LOGICAL_PRESENTATION_LETTERBOX,
		SDL_SCALEMODE_NEAREST);

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

	ImageAsset* ShipObjectsSpriteSheetImageAsset =
		(ImageAsset*)LoadAsset(AssetType_Image, "assets/spritesheets/ship_objects/ship_objects.png");
	ImageAsset* BackgroundObjectsSpriteSheetImageAsset =
		(ImageAsset*)LoadAsset(AssetType_Image, "assets/CelestialObjects.png");

	SpriteSheetAsset* ShipObjectsSpriteSheetDataAsset =
		(SpriteSheetAsset*)LoadAsset(AssetType_SpriteSheetData, "assets/spritesheets/ship_objects/ship_objects.json");

	SpriteDatabaseInitialize(GGame.Renderer);
	GGame.ShipObjectsSpriteSheetHandle =
		SpriteDatabaseCreateFrameDataSpriteSheet(ShipObjectsSpriteSheetImageAsset, ShipObjectsSpriteSheetDataAsset);
	GGame.BackgroundObjectsSpriteSheetHandle =
		SpriteDatabaseCreateGridSpriteSheet(BackgroundObjectsSpriteSheetImageAsset, 32, 32);

	DrawInitialize(&(DrawConfig){
		.Renderer = GGame.Renderer,
	});

	LogInfo("Game Systems Initialized");

	GGame.World = CreateGameWorld();

	if (!GGame.World) {
		LogError("Shit");
		exit(1);
	}

	GGame.LevelEntity = CreateEntity(GGame.World);
	*AddComponent(RenderTintComponent, GGame.World, GGame.LevelEntity) = (RenderTintComponent){
		.TintColor = V4(0.01f, 0.02f, 0.1f, 1.0f),
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
		.SpriteId = SpriteFindByName("projectile01-3"),
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

	*AddComponent(DamageSourceComponent, World, Entity) = (DamageSourceComponent){
		.DamageAmount = 1.0f,
	};

	return Entity;
}

static EntityId CreatePlayerShip(GameWorld* World, Vec2 Position)
{
	LogInfo(__FUNCTION__);

	EntityId Entity = CreateEntity(World);

	NameEntity(World, Entity, "PlayerShip");
	*AddComponent(TransformComponent, World, Entity) = (TransformComponent){
		.Position = Position,
		.Rotation = 0.25f,
	};
	AddComponent(VelocityComponent, World, Entity);
	*AddComponent(SpriteComponent, World, Entity) = (SpriteComponent){
		.SpriteId = SpriteFindByName("darkgrey_06"),
	};
	*AddComponent(ColliderComponent, World, Entity) = (ColliderComponent){
		.Type = ColliderType_Polygon,
		.Group = Group_Friendly,
		.Polygon = PolygonCreateBox(V2(20.0f, 22.0f), V2(0, 0), 0.0f),
	};

	EntityId AnchorEntity = CreateEntity(World);
	// TODO: Find this via queries or something
	NameEntity(World, AnchorEntity, "Anchor");
	GGame.PlayerAnchorEntity = AnchorEntity;
	AddComponent(TransformComponent, World, AnchorEntity);
	AddComponent(LocalTransformComponent, World, AnchorEntity);
	AddComponent(ChildOfComponent, World, AnchorEntity)->Parent = Entity;

	EntityId DroneEntity0 = CreateEntity(World);

	NameEntity(World, DroneEntity0, "Drone0");
	AddComponent(TransformComponent, World, DroneEntity0);
	AddComponent(ChildOfComponent, World, DroneEntity0)->Parent = AnchorEntity;
	AddComponent(LocalTransformComponent, World, DroneEntity0)->LocalPosition = V2(32, 0);
	AddComponent(SpriteComponent, World, DroneEntity0)->SpriteId = SpriteFindByName("mini_1");

	EntityId DroneEntity1 = CreateEntity(World);

	NameEntity(World, DroneEntity1, "Drone1");
	AddComponent(TransformComponent, World, DroneEntity1);
	AddComponent(ChildOfComponent, World, DroneEntity1)->Parent = AnchorEntity;
	AddComponent(LocalTransformComponent, World, DroneEntity1)->LocalPosition = V2(-32, 0);
	AddComponent(SpriteComponent, World, DroneEntity1)->SpriteId = SpriteFindByName("mini_1");

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
	SpriteDatabaseShutdown();
	DrawShutdown();
	LogInfo("Destrying Renderer");
	SDL_DestroyRenderer(GGame.Renderer);
	AssetsShutdown();
	DebugShutdown();
	DestroyGameWorld(GGame.World);
	StringIdPoolsShutdown();
	FrameAllocatorShutdown();

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
		if (EntityIdIsValid(GGame.World, GGame.PlayerEntity)) {
			DestroyEntity(GGame.World, GGame.PlayerEntity);
			LogError("DELETED!");
		}
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
			flt32 ParentRotation = 0.0f;
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

	GGame.FramesThisSecond++;
	GGame.SecondTimer += gameTime->DeltaTimeF;
	if (GGame.SecondTimer >= 1.0f) {
		GGame.SecondTimer -= 1.0f;
		GGame.LastFPS = GGame.FramesThisSecond;
		GGame.FramesThisSecond = 0;
	}
	DebugPrintf("FPS: %d, SIM: %0.3fms", GGame.LastFPS, gameTime->SimTimeMS);
	DebugPrintf("Entities: %d", WorldEntityCount(GGame.World));
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

	int RenderWidth, RenderHeight;
	SDL_GetRenderLogicalPresentation(GGame.Renderer, &RenderWidth, &RenderHeight, NULL, NULL);

	RenderTintComponent* BackgroundTint = TryGetComponent(RenderTintComponent, GGame.World, GGame.LevelEntity);
	if (BackgroundTint != NULL) {
		SDL_FRect ScreenRect = {0, 0, RenderWidth, RenderHeight};
		ColorU8 Color = ColorV4ToColorU8(BackgroundTint->TintColor);
		SDL_SetRenderDrawColor(GGame.Renderer, Color.R, Color.G, Color.B, Color.A);
		SDL_RenderFillRect(GGame.Renderer, &ScreenRect);
	}

	SpriteSystemRender(GGame.World);
	ColliderSystemDebugRender(GGame.World);

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

void ApplyPlayerControl(GameWorld* World, const GameTime* Time, EntityId Entity)
{
	if (EntityIdIsValid(World, GGame.PlayerEntity)) {
		VelocityComponent* V = GetComponent(VelocityComponent, World, GGame.PlayerEntity);
		Vec2 MoveXY = InputXY(SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN);
		V->Velocity = Mul(Norm(MoveXY), 84.0f);
		// V->AngularVelocity = 0.1f;
		GetComponent(LocalTransformComponent, World, GGame.PlayerAnchorEntity)->LocalRotation +=
			Time->DeltaTimeF * 0.1f;
		if (GGame.Timer > 0.0f) GGame.Timer -= Time->DeltaTimeF;
		if (InputKey(SDL_SCANCODE_Z) && GGame.Timer <= 0.0f) {
			GGame.Timer += 0.18f;
			TransformComponent* T = GetComponent(TransformComponent, World, GGame.PlayerEntity);
			Vec2 SpawnPosition = Add(T->Position, V2(16.0f, 0.0));
			flt32 SpawnRotation = T->Rotation - 0.25f;
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
			.TintColor = (Tint != NULL) ? ColorV4ToColorU8(Tint->TintColor) : (ColorU8){0},
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
