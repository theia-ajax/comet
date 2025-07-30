#pragma once

#include "Types.h"

#define RND_U32 uint32
#define RND_U64 uint64
#include "rnd.h"

void RandomSetSeed(uint32 Seed);

uint32 RandomNext(void);
float32 RandomNextF(void);
int32 RandomRange(int32 Min, int32 Max);
float32 RandomRangeF(float32 Min, float32 Max);