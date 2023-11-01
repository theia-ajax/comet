#include "GameState.h"

#include "Log.h"
#include "stb_ds.h"

#define KEntityIndexBits 16
#define KEntityIndexMask ((1 << (KEntityIndexBits - 1)) - 1)
#define ENTITY_ID(Index, Generation)                                                                                   \
	(EntityId)                                                                                                         \
	{                                                                                                                  \
		((Index) & KEntityIndexMask) | ((Generation) << KEntityIndexBits)                                              \
	}
#define ENTITY_ID_INDEX(Entity) ((Entity.RawValue) & KEntityIndexMask)
#define ENTITY_ID_GENERATION(Entity) ((Entity.RawValue) >> KEntityIndexBits)
#define ENTITY_ID_INVALID()                                                                                            \
	(EntityId)                                                                                                         \
	{                                                                                                                  \
		0                                                                                                              \
	}
#define ENTITY_ID_EQ(A, B) ((A).RawValue == (B).RawValue)

int32 BinarySearch(int32 Find, int32* Data, int32 Count)
{
	int32 Result = NONE;

	int32 Head = 0, Tail = Count, Mid = 0;
	while (Head < Tail) {
		Mid = (Tail - Head) / 2 + Head;
		if (Find < Data[Mid]) {
			Tail = Mid;
		} else if (Find > Data[Mid]) {
			Head = Mid + 1;
		} else {
			while (Mid > 0 && Data[Mid - 1] == Find) {
				Mid--;
			}
			Result = Head = Tail = Mid;
			break;
		}
	}

	return Result;
}

int32 BinaryInsertionIndex(int32 Find, int32* Data, int32 Count)
{
	int32 Result = NONE;

	int32 Head = 0, Tail = Count, Mid = 0;
	while (Head < Tail) {
		Mid = (Tail - Head) / 2 + Head;
		if (Find < Data[Mid]) {
			Tail = Mid;
		} else if (Find > Data[Mid]) {
			Head = Mid + 1;
		} else {
			while (Mid > 0 && Data[Mid - 1] == Find) {
				Mid--;
			}
			Result = Head = Tail = Mid;
			break;
		}
	}

	if (Result == NONE && Head == Tail) {
		Result = Head;
	}

	return Result;
}

#define ComponentList(Type) struct { Type* Components; int32* Indices; }
#define ComponentListElemSize(List) sizeof(*((List).Components))

typedef struct GameState {
	GameEntity* Entities;
	EntityId* ActiveEntities;
	int32* EntityGenerations;
	int32* AvailableIndexStack;

	ComponentList(Vec2) Position;

	// New band/album name
	int32 AllTimeHighGeneration;
	int32 AllTimeHighGenerationFirstIndex;
} GameState;

GameState* CreateGameState(void)
{
	GameState* NewState = (GameState*)malloc(sizeof(GameState));
	ZERO_STRUCT(NewState);

	const size_t KInitialEntityCapacity = 256;
	arrsetcap(NewState->Entities, KInitialEntityCapacity);
	arrsetcap(NewState->ActiveEntities, KInitialEntityCapacity);
	arrsetcap(NewState->EntityGenerations, KInitialEntityCapacity);
	arrsetcap(NewState->AvailableIndexStack, 32);

	arrsetcap(NewState->Position.Components, KInitialEntityCapacity);
	arrsetcap(NewState->Position.Indices, KInitialEntityCapacity);

	NewState->AllTimeHighGeneration = 1;
	NewState->AllTimeHighGenerationFirstIndex = NONE;
}

void DestroyGameState(GameState* Self)
{
	arrfree(Self->Entities);
	arrfree(Self->ActiveEntities);
	arrfree(Self->EntityGenerations);
	arrfree(Self->AvailableIndexStack);
	ZERO_STRUCT(Self);
	free(Self);
}

EntityId GameStateCreateEntity(GameState* Self)
{
	int32 EntityIndex = NONE;
	if (arrlen(Self->AvailableIndexStack) > 0) {
		EntityIndex = arrpop(Self->AvailableIndexStack);
	} else {
		EntityIndex = arrlen(Self->Entities);
		// arrput(Self->Entities, (GameEntity){0});
		arrput(Self->EntityGenerations, Self->AllTimeHighGeneration);
	}
	int32 EntityGeneration = Self->EntityGenerations[EntityIndex];
	EntityId Result = ENTITY_ID(EntityIndex, EntityGeneration);
	arrput(Self->ActiveEntities, Result);
	return Result;
}

void GameStateDestroyEntity(GameState* Self, EntityId Entity)
{
	ASSERT(GameStateEntityIdIsValid(Self, Entity));

	int32 EntityIndex = ENTITY_ID_INDEX(Entity);
	int32 EntityGeneration = ENTITY_ID_GENERATION(Entity);

	arrput(Self->AvailableIndexStack, EntityIndex);
	int32 Search = BinarySearch(Entity.RawValue, (int32*)Self->ActiveEntities, arrlen(Self->ActiveEntities));
	if (Search != NONE) {
		arrdel(Self->ActiveEntities, Search);
	} else {
		LogError("GameState:Entities:DestroyEntity: Could not find entity '%d' in ActiveEntities", Entity.RawValue);
	}

	int32 NextGeneration = EntityGeneration + 1;
	if (NextGeneration <= Self->AllTimeHighGeneration) {
		if (EntityIndex < Self->AllTimeHighGenerationFirstIndex) {
			Self->AllTimeHighGeneration++;
			NextGeneration = Self->AllTimeHighGeneration;
			Self->AllTimeHighGenerationFirstIndex = EntityIndex;
		} else {
			NextGeneration = Self->AllTimeHighGeneration;
		}
	} else {
		Self->AllTimeHighGeneration = NextGeneration;
		Self->AllTimeHighGenerationFirstIndex = EntityIndex;
	}

	Self->EntityGenerations[EntityIndex] = NextGeneration;
}

bool GameStateEntityIdIsValid(GameState* Self, EntityId Entity)
{
	if (ENTITY_ID_EQ(Entity, ENTITY_ID_INVALID())) {

		return false;
	}
	return true;
}

// GameEntity* GameStateTryGetEntity(GameState* Self, EntityId Entity)
// {
// 	if (GameStateEntityIdIsValid(Self, Entity)) {
// 		return GameStateGetEntity(Self, Entity);
// 	}
// 	return NULL;
// }

// GameEntity* GameStateGetEntity(GameState* Self, EntityId Entity)
// {
// 	int32 Id = ENTITY_ID_INDEX(Entity);
// 	int32 Index = Self->EntityIndices[Id];
// 	return &Self->Entities[Index];
// }

GameEntity* GameStateEntityList(GameState* Self)
{
	return Self->Entities;
}

int32 GameStateEntityCount(GameState* Self)
{
	ASSERT(Self);
	return arrlen(Self->Entities);
}