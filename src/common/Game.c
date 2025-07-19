#include "Game.h"

#include "ini.h"
#include <SDL3/SDL.h>
#include <stb_ds.h>
#include <stdio.h>
#include <stdlib.h>

#include "AssetTypes.h"
#include "Debug.h"
#include "Draw.h"
#include "FrameAllocator.h"
#include "GameWorld.h"
#include "Log.h"
#include "Math2D.h"
#include "ParticlePhysics.h"
#include "Physics.h"
#include "Random.h"
#include "SpriteDatabase.h"
#include "StringId.h"
#include "Util.h"

enum {
	Group_Friendly,
	Group_Hostile,
};

enum { KSpriteAnimationMaxFrames = 64 };

typedef struct SpriteAnimationFrameData {
	SpriteId Sprite;
	Vec2 PositionOffset;
	flt32 RotationOffset;
} SpriteAnimationFrameData;

typedef struct SpriteAnimationData {
	SpriteAnimationFrameData Frames[KSpriteAnimationMaxFrames];
	int32 FrameCount;
	flt32 SecondsPerFrame;
} SpriteAnimationData;

typedef struct ParticlePhysicsConfigFile {
	struct {
		PhysicsConfig Physics;
		struct {
			const char* heat_colors_image;
		} Rendering;
	} Particles;
	struct {
		const char* FileName;
	} Meta;
} ParticlePhysicsConfigFile;

typedef struct ParticlePhysicsRenderConfig {
	uint32* HeatRampColors;
} ParticlePhysicsRenderConfig;

static EntityId CreateProjectile(GameWorld* World, Vec2 Position, float32 Rotation, float32 Speed);
static EntityId CreatePlayerShip(GameWorld* World, Vec2 Position);
static EntityId CreateEnemy(GameWorld* World, Vec2 Position);

bool ReadConfigFile(const char* FileName, ParticlePhysicsConfigFile* ConfigOut);
bool CheckConfigFileChanges();

void ApplyPlayerControl(GameWorld* World, const GameTime* Time, EntityId Entity);
void MovementSystemUpdate(GameWorld* World, const GameTime* Time);
void DamageSystemUpdate(GameWorld* World, const GameTime* Time);
void LifetimeSystemUpdate(GameWorld* World, const GameTime* Time);
void BehaviorSystemUpdate(GameWorld* World, const GameTime* Time);
void SpriteSystemRender(GameWorld* World);
void ColliderSystemDebugRender(GameWorld* World);
void ParticlePhysicsRender(SDL_Renderer* Renderer, const ParticlePhysicsRenderConfig* Config);

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
	EntityId LevelEntity;
	float32 Timer;
	float32 SecondTimer;
	int32 LastFPS;
	int32 FramesThisSecond;
	ParticlePhysicsConfigFile ParticlePhysicsConfigFile;
	ParticlePhysicsRenderConfig ParticleRenderConfig;
	bool SpawnersEnabled;
	bool DebugDrawEnabled;
} GGame;

SpriteAnimationData GBossIdleAnimationData;
EntityId GBossEntity;

bool GameInitialize(const GameInitParams* params)
{
#ifdef _DEBUG
	LogLevel LoggingLevel = LogLevel_Info;
#else
	LogLevel LoggingLevel = LogLevel_Warning;
#endif

	// LoggingInitialize(LoggingLevel);
	LogInfo(__FUNCTION__);

	FrameAllocatorInitialize(KILOBYTES(640));

	uint32 RandomSeed = (uint32)SDL_GetPerformanceCounter();
	rnd_pcg_seed(&GGame.RandomGen, RandomSeed);
	uint64 HashtableSeed = (uint64)rnd_pcg_next(&GGame.RandomGen) | (((uint64)rnd_pcg_next(&GGame.RandomGen)) << 32);
	stbds_rand_seed(HashtableSeed);

	StringIdPoolsInitialize();

	int32 GameResWidth = 1440 / 4;
	int32 GameResHeight = 320 / 4;

	LogInfo("Creating Renderer");
	GGame.IsRunning = true;
#ifdef _DEBUG
	GGame.DebugDrawEnabled = true;
#endif
	GGame.DebugDrawEnabled = true;
	GGame.SpawnersEnabled = true;
	GGame.Window = params->Window;
	GGame.Renderer = SDL_CreateRenderer(GGame.Window, NULL);
	SDL_SetRenderLogicalPresentation(GGame.Renderer, GameResWidth, GameResHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	DebugInitialize(&(DebugConfig){
		.CanvasWidth = GameResWidth,
		.CanvasHeight = GameResHeight,
		.Renderer = GGame.Renderer,
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
	ImageAsset* HeatRampImage = (ImageAsset*)LoadAsset(AssetType_Image, "assets/heat_color_ramp.png");

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

	{
		GGame.ParticlePhysicsConfigFile.Particles.Physics = PhysicsDefaultConfig();
		ReadConfigFile("pfxconfig.ini", &GGame.ParticlePhysicsConfigFile);

		PhysicsConfig Config = GGame.ParticlePhysicsConfigFile.Particles.Physics;
		Config.Bounds.ZW = V2(GameResWidth, GameResHeight);
		Config.CellSize = 8.0f;
		Config.HeatForce = V2(0, -160);
		// Config.MaxPhysicsObjects = 2048;
		Config.HeatTransferRate = 0.00025f;
		PhysicsInitialize(&Config);

		for (uint8* Pixel = HeatRampImage->Data->Pixels;
			 Pixel != HeatRampImage->Data->Pixels + (HeatRampImage->Data->Width * HeatRampImage->Data->BytesPerPixel);
			 Pixel += HeatRampImage->Data->BytesPerPixel)
		{
			uint32 Color = *((uint32*)Pixel);
			arrput(GGame.ParticleRenderConfig.HeatRampColors, Color);
			// SDL_Log("%08x", Color);
		}
	}

	LogInfo("Game Systems Initialized");

	GGame.World = CreateGameWorld();

	if (!GGame.World) {
		LogError("Shit");
		exit(1);
	}

	GGame.LevelEntity = CreateEntity(GGame.World);
	*AddComponent(RenderTintComponent, GGame.World, GGame.LevelEntity) = (RenderTintComponent){
		.TintColor = V4(0.0f, 0.0f, 0.0f, 1.0f),
	};

	// GGame.PlayerEntity = CreatePlayerShip(GGame.World, V2(64, 128));
	// CreateEnemy(GGame.World, V2(256, 128));

	// {
	// 	EntityId BackgroundEntity = CreateEntity(GGame.World);
	// 	*AddComponent(TransformComponent, GGame.World, BackgroundEntity) = (TransformComponent){
	// 		.Position = V2(372, 128),
	// 	};
	// 	*AddComponent(SpriteComponent, GGame.World, BackgroundEntity) = (SpriteComponent){
	// 		.SpriteId = SpriteSheetFindSpriteByIndex(GGame.BackgroundObjectsSpriteSheetHandle, 44),
	// 	};
	// 	*AddComponent(SpriteTilesComponent, GGame.World, BackgroundEntity) = (SpriteTilesComponent){
	// 		.Tiles = {4, 4},
	// 	};
	// 	*AddComponent(RenderLayerComponent, GGame.World, BackgroundEntity) = (RenderLayerComponent){
	// 		.Layer = -1000,
	// 	};
	// }

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

	GBossEntity = CreateEntity(World);
	AddComponent(TransformComponent, World, GBossEntity)->Position = V2(600, 100);
	AddComponent(SpriteComponent, World, GBossEntity);
	AddComponent(TimerComponent, World, GBossEntity);

	LogInfo("Game Initialization Complete");

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
	switch (event->type) {
		case SDL_EVENT_KEY_DOWN:
			switch (event->key.scancode) {
				case SDL_SCANCODE_F1: GGame.DebugDrawEnabled = !GGame.DebugDrawEnabled; break;
				case SDL_SCANCODE_F:
					if (EntityIdIsValid(GGame.World, GGame.PlayerEntity)) {
						DestroyEntity(GGame.World, GGame.PlayerEntity);
						LogError("DELETED!");
					}
					break;
				case SDL_SCANCODE_R:
					PhysicsClearAllObjects();
					GGame.SpawnersEnabled = true;
					break;
				case SDL_SCANCODE_S: GGame.SpawnersEnabled = !GGame.SpawnersEnabled; break;
				default: break;
			}
			break;
		default: break;
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

float32 Spawners[] = {
	24, 8, 1000, 100000, 0, 16, 8, 1000, 100000, 0, 360 - 24, 8, -1000, 100000, 0, 360 - 16, 8, -1000, 100000, 0,
	// 1440 - 24,
	// 100,
	// -1000000,
	// 25000,
};

const float32 SpawnInterval = 1.0f / 15.0f;

void GameUpdate(const GameTime* gameTime)
{
	// FrameAllocatorNextFrame();
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

	// if (GGame.Frame == 1 && false) {
	// 	const float32 SpacingX = 16.0f;
	// 	const float32 SpacingY = 12.0f;
	// 	PhysicsConfig Config = *PhysicsGetConfig();
	// 	for (float32 y = Config.Bounds.Y + Config.CellSize + 84.0f; y < Config.Bounds.W - Config.CellSize; y +=
	// 																									   SpacingX)
	// 	{
	// 		for (float32 x = Config.Bounds.X + Config.CellSize; x < Config.Bounds.Z - Config.CellSize; x += SpacingY) {
	// 			PhysicsAddObject(&(PhysicsObject){
	// 				.Position = V2(x, y),
	// 				.Radius = 3.0f,
	// 				.Heat = rnd_pcg_nextf(&GGame.RandomGen),
	// 				.Acceleration = V2(10000 * (rnd_pcg_nextf(&GGame.RandomGen) < 0.5f ? -1.0f : 1.0f), 0),
	// 			});
	// 		}
	// 	}
	// }

	if (GGame.SpawnersEnabled) {

		for (int32 SpawnerIndex = 0; SpawnerIndex < ARRAY_COUNT(Spawners); SpawnerIndex += 5) {
			float32* Spawner = &Spawners[SpawnerIndex];
			Vec2 SpawnPos = V2(Spawner[0], Spawner[1]);
			Vec2 SpawnAccel = V2(Spawner[2], Spawner[3]);
			float32* SpawnTimer = &Spawner[4];
			if (*SpawnTimer <= 0.0f) {
				*SpawnTimer += SpawnInterval;
				PhysicsAddObject(&(PhysicsObject){
					.Position = SpawnPos,
					.Radius = 1.0f,
					.Acceleration = SpawnAccel,
					.Heat = 0.0f,
					.Flags = PhysicsObjectFlags_None,
					.Tint = 0xFFCC00CC,
				});
			} else {
				*SpawnTimer -= gameTime->DeltaTimeF;
			}
		}

		if (PhysicsGetObjectCount() >= PhysicsGetConfig()->MaxPhysicsObjects) {
			GGame.SpawnersEnabled = false;
		}
	}

	PhysicsUpdate(gameTime->DeltaTimeF);

	GGame.FramesThisSecond++;
	GGame.SecondTimer += gameTime->DeltaTimeF;
	if (GGame.SecondTimer >= 1.0f) {
		GGame.SecondTimer -= 1.0f;
		GGame.LastFPS = GGame.FramesThisSecond;
		GGame.FramesThisSecond = 0;
	}
	DebugPrintf("FPS: %d, SIM: %0.3fms", GGame.LastFPS, gameTime->SimTimeMS);
	DebugPrintf("Entities: %d", WorldEntityCount(GGame.World));
	DebugPrintf("Objects: %d/%d", PhysicsGetObjectCount(), PhysicsGetConfig()->MaxPhysicsObjects);
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
	SDL_GetRenderLogicalPresentation(GGame.Renderer, &RenderWidth, &RenderHeight, NULL);

	RenderTintComponent* BackgroundTint = TryGetComponent(RenderTintComponent, GGame.World, GGame.LevelEntity);
	if (BackgroundTint != NULL) {
		SDL_FRect ScreenRect = {0, 0, RenderWidth, RenderHeight};
		ColorU8 Color = ColorU8FromVec4(BackgroundTint->TintColor);
		SDL_SetRenderDrawColor(GGame.Renderer, Color.R, Color.G, Color.B, Color.A);
		SDL_RenderFillRect(GGame.Renderer, &ScreenRect);
	}

	SpriteSystemRender(GGame.World);
	ColliderSystemDebugRender(GGame.World);

	ParticlePhysicsRender(GGame.Renderer, &GGame.ParticleRenderConfig);

	DrawRender();

	// PhysicsDebugDraw(GGame.Renderer);

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

int PhysicsObjectDrawOrderCompare(const void* A, const void* B)
{
	const PhysicsObject* ObjA = (const PhysicsObject*)A;
	const PhysicsObject* ObjB = (const PhysicsObject*)B;

	if (ObjA->Position.Y < ObjB->Position.Y) {
		return -1;
	} else if (ObjA->Position.Y > ObjB->Position.Y) {
		return 1;
	} else {
		return 0;
	}
}

void ParticlePhysicsRender(SDL_Renderer* Renderer, const ParticlePhysicsRenderConfig* Config)
{
	const PhysicsObject* Objects = PhysicsGetObjects();
	for (size_t Index = 0; Index < PhysicsGetObjectCount(); Index++) {
		const PhysicsObject* Object = &Objects[Index];
		Vec2 Pos = Object->Position;
		float32 Radius = Object->Radius;
		SDL_FRect PosRect = {Pos.X - Radius, Pos.Y - Radius, Radius * 2 + 1, Radius * 2 + 1};
		uint32 TintColor;
		if ((Object->Flags & 1) != 0) {
			TintColor = Object->Tint;
		} else {
			int32 Index = (int32)(MIN(Object->Heat, 1.0f - KEpsilonFloat32) * arrlen(Config->HeatRampColors));
			TintColor = Config->HeatRampColors[Index];
		}

		float32 Scale = (Radius + Object->Heat * 4.0f + 4.0f) / 26.0f;

		// DrawCircle(Pos, Scale, TintColor);
		DrawSprite(&(SpriteDraw){
			.SpriteId = SpriteFindByName("explosion-01"),
			.Position = Pos,
			.Scale = V2(Scale, Scale),
			.UseTint = true,
			.TintColor = ColorU8FromColorU32(TintColor),
		});
		// SDL_SetRenderDrawColor(GGame.Renderer, R, G, B, A);
		// SDL_RenderFillRect(GGame.Renderer, &PosRect);
	}
}

bool ReadConfigFile(const char* FileName, ParticlePhysicsConfigFile* ConfigOut)
{
	FILE* File = fopen(FileName, "r");

	if (!File) {
		return false;
	}

	fseek(File, 0, SEEK_END);
	long FileLength = ftell(File);
	fseek(File, 0, SEEK_SET);

	char* FileData = (char*)calloc(FileLength + 1, sizeof(char));

	size_t BytesRead = fread(FileData, sizeof(char), FileLength, File);
	fclose(File);
	ini_t* IniFile = ini_load(FileData, NULL);
	free(FileData);

	if (!IniFile) {
		return false;
	}

	int PhysicsSection = ini_find_section(IniFile, "Physics", 0);

	if (PhysicsSection != INI_NOT_FOUND) {
		int MaxObjectCountProperty = ini_find_property(IniFile, PhysicsSection, "MaxObjectCount", 0);
		if (MaxObjectCountProperty != INI_NOT_FOUND) {
			const char* PropertyValue = ini_property_value(IniFile, PhysicsSection, MaxObjectCountProperty);
			long IntValue = strtol(PropertyValue, NULL, 10);
			ConfigOut->Particles.Physics.MaxPhysicsObjects = IntValue;
		}
	}

	ini_destroy(IniFile);

	return true;
}
