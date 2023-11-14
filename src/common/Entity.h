#pragma once

#include "Types.h"

#define KEntityIndexBits 16
#define KEntityIndexMask ((1 << (KEntityIndexBits - 1)) - 1)

typedef struct EntityId {
	int32 RawValue;
} EntityId;

// clang-format off
#define ENTITY_ID(Index, Generation) (EntityId) { ((Index) & KEntityIndexMask) | ((Generation) << KEntityIndexBits) }
#define ENTITY_ID_INDEX(Entity) ((Entity.RawValue) & KEntityIndexMask)
#define ENTITY_ID_GENERATION(Entity) ((Entity.RawValue) >> KEntityIndexBits)
#define ENTITY_ID_INVALID (EntityId){0}
#define ENTITY_ID_EQ(A, B) ((A).RawValue == (B).RawValue)
#define ENTITY_ID_NEQ(A, B) ((A).RawValue != (B).RawValue)
// clang-format on

static inline int32 EntityIdCompare(EntityId A, EntityId B) { return A.RawValue - B.RawValue; }

typedef struct EntitySignature {
	uint64 RawValue;
} EntitySignature;
