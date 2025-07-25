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
		int32 Width;
		int32 Height;
		StringId RenderDriver;
		StringId HeatColorsImageFileName;
		float32 SpriteScale;
		float32 ExtraRadius;
		float32 HeatScale;
		StringId ParticleSpriteName;
		StringId ParticleSpriteSheetName;
		int32 ParticleSpriteIndex;
		bool UseSpriteIndex;
		bool UseHeatAsLayer;
		bool InvertLayer;
	} Rendering;
	struct {
		Vec2 Offset;
		float32 Spacing;
		float32 Interval;
		float32 ObjectRadius;
	} Spawners;
	PhysicsConfig Physics;
	struct {
		const char* FileName;
		SDL_Time LastModified;
	} Meta;
} ParticlePhysicsConfigFile;

typedef struct ParticlePhysicsRenderConfig {
	uint32* HeatRampColors;
	SpriteSheetId ParticleSpriteSheetId;
	SpriteId ParticleSpriteId;
} ParticlePhysicsRenderConfig;

typedef struct ParticleSpawner {
	Vec2 Position;
	Vec2 SpawnAcceleration;
	float32 SpawnInterval;
	float32 SpawnTimer;
	float32 ObjectRadius;
} ParticleSpawner;

static EntityId CreateProjectile(GameWorld* World, Vec2 Position, float32 Rotation, float32 Speed);
static EntityId CreatePlayerShip(GameWorld* World, Vec2 Position);
static EntityId CreateEnemy(GameWorld* World, Vec2 Position);

// Allocates new buffer to read file into, returns true if succesfully read file and *OutFileData will point to the
// allocated buffer. The caller is responsible for freeing this buffer!
bool ReadFileToNewBuffer(const char* FileName, char** OutFileData);
bool ReadConfigFile(const char* FileName, ParticlePhysicsConfigFile* ConfigOut);
bool CheckConfigFileChanges();
void LoadHeatRamp(const char* HeatRampFileName);
void UpdateParticleSpriteId();
void CreateSpawners();

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
	SDL_Texture* ParticleRenderTexture;
	int32 GameResWidth;
	int32 GameResHeight;
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
	ParticlePhysicsConfigFile ParticlePhysicsConfigFile;
	ParticlePhysicsRenderConfig ParticleRenderConfig;
	ParticleSpawner* Spawners;
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

	LoggingLevel = LogLevel_Info;
	LoggingInitialize(LoggingLevel);
	LogInfo(__FUNCTION__);

	FrameAllocatorInitialize(KILOBYTES(640));

	uint32 RandomSeed = (uint32)SDL_GetPerformanceCounter();
	rnd_pcg_seed(&GGame.RandomGen, RandomSeed);
	uint64 HashtableSeed = (uint64)rnd_pcg_next(&GGame.RandomGen) | (((uint64)rnd_pcg_next(&GGame.RandomGen)) << 32);
	stbds_rand_seed(HashtableSeed);

	StringIdPoolsInitialize();

	GGame.ParticlePhysicsConfigFile.Physics = PhysicsDefaultConfig();
	ReadConfigFile("particles.ini", &GGame.ParticlePhysicsConfigFile);

	GGame.GameResWidth = GGame.ParticlePhysicsConfigFile.Rendering.Width;
	GGame.GameResHeight = GGame.ParticlePhysicsConfigFile.Rendering.Height;

	const char* RenderDriverName = StringIdCStr(GGame.ParticlePhysicsConfigFile.Rendering.RenderDriver);
	const char* SelectedRenderDriver = NULL;
	const char* DefaultRenderDriver = NULL;

	for (int DriverIndex = 0; DriverIndex < SDL_GetNumRenderDrivers(); DriverIndex++) {
		const char* Driver = SDL_GetRenderDriver(DriverIndex);
		if (DriverIndex == 0) {
			DefaultRenderDriver = Driver;
		}
		if (SDL_strcasecmp(RenderDriverName, Driver) == 0) {
			SelectedRenderDriver = Driver;
			break;
		}
	}

	SelectedRenderDriver = SelectedRenderDriver ? SelectedRenderDriver : DefaultRenderDriver;

	LogInfo("Creating renderer with '%s' driver.", SelectedRenderDriver);
	GGame.IsRunning = true;
#ifdef _DEBUG
	GGame.DebugDrawEnabled = true;
#endif
	GGame.SpawnersEnabled = true;
	GGame.Window = params->Window;
	GGame.Renderer = SDL_CreateRenderer(GGame.Window, SelectedRenderDriver);
	int WindowWidth, WindowHeight;
	SDL_GetWindowSizeInPixels(GGame.Window, &WindowWidth, &WindowHeight);
	SDL_SetRenderLogicalPresentation(GGame.Renderer, WindowWidth, WindowHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	SDL_PropertiesID RendererProperties = SDL_GetRendererProperties(GGame.Renderer);
	SDL_PixelFormat* RendererPixelFormats = SDL_GetPointerProperty(
		SDL_GetRendererProperties(GGame.Renderer),
		SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER,
		NULL);

	SDL_PixelFormat Format = SDL_PIXELFORMAT_UNKNOWN;
	if (RendererPixelFormats) {
		Format = RendererPixelFormats[0];
	}

	GGame.ParticleRenderTexture =
		SDL_CreateTexture(GGame.Renderer, Format, SDL_TEXTUREACCESS_TARGET, GGame.GameResWidth, GGame.GameResHeight);
	SDL_SetTextureScaleMode(GGame.ParticleRenderTexture, SDL_SCALEMODE_LINEAR);

	DebugInitialize(&(DebugConfig){
		.CanvasWidth = WindowWidth / 4,
		.CanvasHeight = 16,
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
	ImageAsset* FlaresSpriteSheetImageAsset = (ImageAsset*)LoadAsset(AssetType_Image, "assets/flares.png");
	ImageAsset* BigFlaresSpriteSheetImageAsset = (ImageAsset*)LoadAsset(AssetType_Image, "assets/bigflare.png");

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

	DrawInitialize(&(DrawConfig){
		.Renderer = GGame.Renderer,
	});

	PhysicsInitialize(&GGame.ParticlePhysicsConfigFile.Physics);
	LoadHeatRamp(StringIdCStr(GGame.ParticlePhysicsConfigFile.Rendering.HeatColorsImageFileName));
	UpdateParticleSpriteId();

	CreateSpawners();

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

void GameUpdate(const GameTime* gameTime)
{
	FrameAllocatorNextFrame();
	DebugNextFrame();

	CheckConfigFileChanges();

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

	// Update Spawners
	if (GGame.SpawnersEnabled) {
		for (int32 SpawnerIndex = 0; SpawnerIndex < arrlen(GGame.Spawners); SpawnerIndex += 5) {
			ParticleSpawner* Spawner = &GGame.Spawners[SpawnerIndex];

			if (Spawner->SpawnTimer <= 0.0f) {
				Spawner->SpawnTimer += Spawner->SpawnInterval;
				Vec2 Accel = Spawner->SpawnAcceleration;
				Accel.X += rnd_pcg_nextf(&GGame.RandomGen) * 10000.0f - 5000.0f;
				PhysicsAddObject(&(PhysicsObject){
					.Position = Spawner->Position,
					.Radius = Spawner->ObjectRadius,
					.Acceleration = Accel,
					.Heat = 0.0f,
					.Flags = PhysicsObjectFlags_None,
					.Tint = 0xFFCC00CC,
				});
			} else {
				Spawner->SpawnTimer -= gameTime->DeltaTimeF;
			}
		}

		if (PhysicsGetObjectCount() >= PhysicsGetConfig()->MaxPhysicsObjects) {
			GGame.SpawnersEnabled = false;
		}
	}

	PhysicsUpdate(1/60.0f);

	GGame.FramesThisSecond++;
	GGame.SecondTimer += gameTime->DeltaTimeF;
	if (GGame.SecondTimer >= 1.0f) {
		GGame.SecondTimer -= 1.0f;
		GGame.LastFPS = GGame.FramesThisSecond;
		GGame.FramesThisSecond = 0;
	}
	DebugPrintf("FPS: %d, SIM: %0.3fms, DRAW: %0.3fms", GGame.LastFPS, gameTime->SimTimeMS, gameTime->RenderTimeMS);
	// DebugPrintf("Entities: %d", WorldEntityCount(GGame.World));
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

	RenderTintComponent* BackgroundTint = TryGetComponent(RenderTintComponent, GGame.World, GGame.LevelEntity);
	if (BackgroundTint != NULL) {
		ColorU8 Color = ColorU8FromVec4(BackgroundTint->TintColor);
		SDL_SetRenderDrawColor(GGame.Renderer, Color.R, Color.G, Color.B, Color.A);
		SDL_RenderFillRect(GGame.Renderer, NULL);
	}

	SpriteSystemRender(GGame.World);
	ColliderSystemDebugRender(GGame.World);

	SDL_SetRenderTarget(GGame.Renderer, GGame.ParticleRenderTexture);

	SDL_SetRenderDrawColor(GGame.Renderer, 0, 0, 0, 0);
	SDL_RenderClear(GGame.Renderer);

	ParticlePhysicsRender(GGame.Renderer, &GGame.ParticleRenderConfig);

	DrawRender();

	SDL_SetRenderTarget(GGame.Renderer, NULL);

	SDL_RenderTexture(GGame.Renderer, GGame.ParticleRenderTexture, NULL, NULL);

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
			ptrdiff_t len = arrlen(Config->HeatRampColors);
			if (len > 0) {
				int32 Index = (int32)(MIN(Object->Heat, 1.0f - KEpsilonFloat32) * len);
				TintColor = Config->HeatRampColors[Index];
			} else {
				TintColor = Object->Tint;
			}
		}

		float32 SpriteScale = GGame.ParticlePhysicsConfigFile.Rendering.SpriteScale;
		float32 HeatScale = GGame.ParticlePhysicsConfigFile.Rendering.HeatScale;
		float32 ExtraRadius = GGame.ParticlePhysicsConfigFile.Rendering.ExtraRadius;
		float32 Scale = (Radius + Object->Heat * HeatScale + ExtraRadius) * SpriteScale;
		float32 Layer =
			GGame.ParticlePhysicsConfigFile.Rendering.UseHeatAsLayer
				? ((GGame.ParticlePhysicsConfigFile.Rendering.InvertLayer) ? 1.0f - Object->Heat : Object->Heat)
				: 0;

		// DrawCircle(Pos, Scale, TintColor);
		DrawSprite(&(SpriteDraw){
			.SpriteId = GGame.ParticleRenderConfig.ParticleSpriteId,
			.Position = Pos,
			.Scale = V2(Scale, Scale),
			.UseTint = true,
			.TintColor = ColorU8FromColorU32(TintColor),
			.Layer = Layer,
		});
		// SDL_SetRenderDrawColor(GGame.Renderer, R, G, B, A);
		// SDL_RenderFillRect(GGame.Renderer, &PosRect);
	}
}

// Allocates new buffer to read file into, returns true if succesfully read file and *OutFileData will point to the
// allocated buffer. The caller is responsible for freeing this buffer!
bool ReadFileToNewBuffer(const char* FileName, char** OutFileData)
{
	FILE* File = fopen(FileName, "r");
	*OutFileData = NULL;

	if (!File) {
		return false;
	}

	fseek(File, 0, SEEK_END);
	long FileLength = ftell(File);
	fseek(File, 0, SEEK_SET);

	char* FileData = (char*)calloc(FileLength + 1, sizeof(char));

	if (!FileData) {
		fclose(File);
		return false;
	}

	size_t BytesRead = fread(FileData, sizeof(char), FileLength, File);
	fclose(File);

	*OutFileData = FileData;
	return true;
}

bool IniHasSection(ini_t* Ini, const char* Section)
{
	return ini_find_section(Ini, Section, 0) != INI_NOT_FOUND;
}

bool IniHasProperty(ini_t* Ini, int Section, const char* Property)
{
	return ini_find_property(Ini, Section, Property, 0) != INI_NOT_FOUND;
}

const char* IniReadString(ini_t* Ini, int Section, const char* Property, const char* Default)
{
	const char* Result = Default;
	int PropertyIndex = ini_find_property(Ini, Section, Property, 0);
	if (PropertyIndex != INI_NOT_FOUND) {
		Result = ini_property_value(Ini, Section, PropertyIndex);
	}
	return Result;
}

StringId IniReadStringId(ini_t* Ini, int Section, const char* Property, StringId Default)
{
	StringId Result = KInvalidStringId;
	const char* StringValue = IniReadString(Ini, Section, Property, NULL);
	if (StringValue) {
		Result = GetStringId(StringValue);
	}
	return Result;
}

int IniReadInt(ini_t* Ini, int Section, const char* Property, int Default)
{
	int Result = Default;

	const char* Value = IniReadString(Ini, Section, Property, NULL);
	if (Value) {
		Result = strtol(Value, NULL, 10);
	}
	return Result;
}

double IniReadFloat(ini_t* Ini, int Section, const char* Property, double Default)
{
	double Result = Default;

	const char* Value = IniReadString(Ini, Section, Property, NULL);
	if (Value) {
		Result = strtod(Value, NULL);
	}
	return Result;
}

bool IniReadBool(ini_t* Ini, int Section, const char* Property, bool Default)
{
	bool Result = Default;

	const char* Value = IniReadString(Ini, Section, Property, NULL);
	if (Value) {
		Result = Value[0] == 't' || Value[0] == 'T';
	}
	return Result;
}

bool ReadConfigFile(const char* FileName, ParticlePhysicsConfigFile* ConfigOut)
{
	char* FileData;
	if (ReadFileToNewBuffer(FileName, &FileData)) {
		ini_t* Ini = ini_load(FileData, NULL);
		free(FileData);

		if (!Ini) {
			return false;
		}

		ConfigOut->Meta.FileName = FileName;

		if (ConfigOut->Meta.LastModified == 0) {
			SDL_PathInfo PathInfo;
			SDL_Time LastModifiedTime = 0;
			if (SDL_GetPathInfo(FileName, &PathInfo)) {
				LastModifiedTime = PathInfo.modify_time;
			}
			ConfigOut->Meta.LastModified = LastModifiedTime;
		}

		{
			int Section = ini_find_section(Ini, "Rendering", 0);

			ConfigOut->Rendering.Width = IniReadInt(Ini, Section, "Width", 360);
			ConfigOut->Rendering.Height = IniReadInt(Ini, Section, "Height", 80);
			ConfigOut->Rendering.RenderDriver = IniReadStringId(Ini, Section, "RenderDriver", KInvalidStringId);
			ConfigOut->Rendering.HeatColorsImageFileName =
				IniReadStringId(Ini, Section, "HeatColorsImage", KInvalidStringId);
			ConfigOut->Rendering.SpriteScale = IniReadFloat(Ini, Section, "SpriteScale", 1.0);
			ConfigOut->Rendering.HeatScale = IniReadFloat(Ini, Section, "HeatScale", 0.0);
			ConfigOut->Rendering.ExtraRadius = IniReadFloat(Ini, Section, "ExtraRadius", 0.0);
			ConfigOut->Rendering.ParticleSpriteName =
				IniReadStringId(Ini, Section, "ParticleSprite", GetStringId("explosion-01"));
			ConfigOut->Rendering.ParticleSpriteSheetName =
				IniReadStringId(Ini, Section, "ParticleSpriteSheet", KInvalidStringId);
			ConfigOut->Rendering.ParticleSpriteIndex = IniReadInt(Ini, Section, "ParticleSpriteIndex", 0);
			ConfigOut->Rendering.UseSpriteIndex = IniReadBool(Ini, Section, "UseSpriteIndex", false);
			ConfigOut->Rendering.UseHeatAsLayer = IniReadBool(Ini, Section, "UseHeatAsLayer", true);
			ConfigOut->Rendering.InvertLayer = IniReadBool(Ini, Section, "InvertLayer", false);
		}

		{
			int Section = ini_find_section(Ini, "Spawners", 0);

			ConfigOut->Spawners.Offset.X = IniReadFloat(Ini, Section, "OffsetX", 0.0);
			ConfigOut->Spawners.Offset.Y = IniReadFloat(Ini, Section, "OffsetY", 0.0);
			ConfigOut->Spawners.Interval = IniReadFloat(Ini, Section, "Interval", 0.0);
			ConfigOut->Spawners.Spacing = IniReadFloat(Ini, Section, "Spacing", 32.0);
			ConfigOut->Spawners.ObjectRadius = IniReadFloat(Ini, Section, "ObjectRadius", 1.0);
		}

		{
			int Section = ini_find_section(Ini, "Physics", 0);
			ConfigOut->Physics.MaxPhysicsObjects = IniReadInt(Ini, Section, "MaxObjectCount", 128);

			ConfigOut->Physics.Bounds.X = IniReadFloat(Ini, Section, "BoundsOffsetX", 0.0);
			ConfigOut->Physics.Bounds.Y = IniReadFloat(Ini, Section, "BoundsOffsetY", 0.0);

			float32 BoundsWidth = IniReadFloat(Ini, Section, "BoundsWidth", GGame.GameResWidth);
			float32 BoundsHeight = IniReadFloat(Ini, Section, "BoundsHeight", GGame.GameResHeight);
			if (BoundsWidth < KEpsilonFloat32) {
				BoundsWidth = GGame.GameResWidth;
			};
			if (BoundsWidth < KEpsilonFloat32) {
				BoundsHeight = GGame.GameResHeight;
			};

			ConfigOut->Physics.Bounds.Z = ConfigOut->Physics.Bounds.X + BoundsWidth;
			ConfigOut->Physics.Bounds.W = ConfigOut->Physics.Bounds.Y + BoundsHeight;

			ConfigOut->Physics.CellSize = IniReadFloat(Ini, Section, "CellSize", 8.0);

			ConfigOut->Physics.Gravity.X = IniReadFloat(Ini, Section, "GravityX", 0.0);
			ConfigOut->Physics.Gravity.Y = IniReadFloat(Ini, Section, "GravityY", 100.0);

			ConfigOut->Physics.HeatForce.X = IniReadFloat(Ini, Section, "HeatForceX", 0.0);
			ConfigOut->Physics.HeatForce.Y = IniReadFloat(Ini, Section, "HeatForceY", -160.0);

			ConfigOut->Physics.HeatTransferRate = IniReadFloat(Ini, Section, "HeatTransferRate", 0.0);

			ConfigOut->Physics.HeatDecay = IniReadFloat(Ini, Section, "HeatDecay", 0.0);
			ConfigOut->Physics.HeaterZoneSize = IniReadFloat(Ini, Section, "HeaterZoneSize", 0.0);
			ConfigOut->Physics.HeaterHeatDelta = IniReadFloat(Ini, Section, "HeaterHeatDelta", 0.0);
			ConfigOut->Physics.CoolerZoneSize = IniReadFloat(Ini, Section, "CoolerZoneSize", 0.0);
			ConfigOut->Physics.CoolerHeatDelta = IniReadFloat(Ini, Section, "CoolerHeatDelta", 0.0);

			ConfigOut->Physics.SquishZoneSize = IniReadFloat(Ini, Section, "SquishZoneSize", 0.0);
			ConfigOut->Physics.SquishZoneForceMin = IniReadFloat(Ini, Section, "SquishZoneForceMin", 0.0);
			ConfigOut->Physics.SquishZoneForceMax =
				IniReadFloat(Ini, Section, "SquishZoneForceMax", ConfigOut->Physics.SquishZoneForceMin);
		}

		ini_destroy(Ini);
	}

	return true;
}

void ApplyConfigFileChanges(const ParticlePhysicsConfigFile* Old, const ParticlePhysicsConfigFile* New)
{
	if (SDL_memcmp(&Old->Physics, &New->Physics, sizeof(Old->Physics)) != 0) {
		if (Old->Physics.MaxPhysicsObjects != New->Physics.MaxPhysicsObjects) {
			if (New->Physics.MaxPhysicsObjects < Old->Physics.MaxPhysicsObjects) {
				PhysicsClearAllObjects();
			}
			GGame.SpawnersEnabled = true;
		}

		PhysicsReconfigure(&New->Physics);
	}

	if (!StringIdEq(Old->Rendering.HeatColorsImageFileName, New->Rendering.HeatColorsImageFileName)) {
		LoadHeatRamp(StringIdCStr(New->Rendering.HeatColorsImageFileName));
	}

	if (!StringIdEq(Old->Rendering.ParticleSpriteName, New->Rendering.ParticleSpriteName) ||
		!StringIdEq(Old->Rendering.ParticleSpriteSheetName, New->Rendering.ParticleSpriteSheetName) ||
		Old->Rendering.ParticleSpriteIndex != New->Rendering.ParticleSpriteIndex)
	{
		UpdateParticleSpriteId();
	}

	if (SDL_memcmp(&Old->Physics.Bounds, &New->Physics.Bounds, sizeof(Old->Physics.Bounds)) != 0 ||
		SDL_memcmp(&Old->Spawners, &New->Spawners, sizeof(Old->Spawners)) != 0)
	{
		CreateSpawners();
	}
}

bool CheckConfigFileChanges()
{
	SDL_PathInfo PathInfo;
	SDL_Time LastModifiedTime = 0;
	if (SDL_GetPathInfo(GGame.ParticlePhysicsConfigFile.Meta.FileName, &PathInfo)) {
		LastModifiedTime = PathInfo.modify_time;
	}

	if (LastModifiedTime > GGame.ParticlePhysicsConfigFile.Meta.LastModified) {
		ParticlePhysicsConfigFile OldConfig = GGame.ParticlePhysicsConfigFile;

		ReadConfigFile(GGame.ParticlePhysicsConfigFile.Meta.FileName, &GGame.ParticlePhysicsConfigFile);
		GGame.ParticlePhysicsConfigFile.Meta.LastModified = LastModifiedTime;

		ApplyConfigFileChanges(&OldConfig, &GGame.ParticlePhysicsConfigFile);
	}
}

void LoadHeatRamp(const char* HeatRampFileName)
{
	ImageAsset* HeatRampImage = (ImageAsset*)LoadAsset(AssetType_Image, HeatRampFileName);
	if (HeatRampImage) {
		arrsetlen(GGame.ParticleRenderConfig.HeatRampColors, 0);

		for (uint8* Pixel = HeatRampImage->Data->Pixels;
			 Pixel != HeatRampImage->Data->Pixels + (HeatRampImage->Data->Width * HeatRampImage->Data->BytesPerPixel);
			 Pixel += HeatRampImage->Data->BytesPerPixel)
		{
			uint32 Color = *((uint32*)Pixel);
			arrput(GGame.ParticleRenderConfig.HeatRampColors, Color);
		}
	}
}

void UpdateParticleSpriteId()
{
	if (StringIdIsValid(GGame.ParticlePhysicsConfigFile.Rendering.ParticleSpriteSheetName)) {
		GGame.ParticleRenderConfig.ParticleSpriteSheetId =
			SpriteSheetFindByName(GGame.ParticlePhysicsConfigFile.Rendering.ParticleSpriteSheetName);

		if (GGame.ParticlePhysicsConfigFile.Rendering.UseSpriteIndex) {
			GGame.ParticleRenderConfig.ParticleSpriteId = SpriteSheetFindSpriteByIndex(
				GGame.ParticleRenderConfig.ParticleSpriteSheetId,
				GGame.ParticlePhysicsConfigFile.Rendering.ParticleSpriteIndex);
		} else {
			GGame.ParticleRenderConfig.ParticleSpriteId = SpriteSheetFindSpriteByNameId(
				GGame.ParticleRenderConfig.ParticleSpriteSheetId,
				GGame.ParticlePhysicsConfigFile.Rendering.ParticleSpriteName);
		}
	} else {
		GGame.ParticleRenderConfig.ParticleSpriteId =
			SpriteFindByNameId(GGame.ParticlePhysicsConfigFile.Rendering.ParticleSpriteName);
	}
}

void CreateSpawners()
{
	arrsetlen(GGame.Spawners, 0);

	for (float32 SpawnerX =
			 GGame.ParticlePhysicsConfigFile.Spawners.Offset.X + GGame.ParticlePhysicsConfigFile.Physics.Bounds.X;
		 SpawnerX < GGame.ParticlePhysicsConfigFile.Physics.Bounds.Z;
		 SpawnerX += Max(GGame.ParticlePhysicsConfigFile.Spawners.Spacing, 1.0f))
	{
		arrput(
			GGame.Spawners,
			((ParticleSpawner){
				.Position = V2(SpawnerX, GGame.ParticlePhysicsConfigFile.Spawners.Offset.Y),
				.SpawnAcceleration = V2(0, 0.0f),
				.SpawnInterval = GGame.ParticlePhysicsConfigFile.Spawners.Interval,
				.ObjectRadius = Max(GGame.ParticlePhysicsConfigFile.Spawners.ObjectRadius, 0.1f),
			}));
	}
}
