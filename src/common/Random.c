#include "Random.h"

#include <HandmadeMath.h>

rnd_pcg_t GRandom;

void RandomSetSeed(uint32 Seed)
{
	rnd_pcg_seed(&GRandom, Seed);
}

uint32 RandomNext(void)
{
	return rnd_pcg_next(&GRandom);
}

float32 RandomNextF(void)
{
	return rnd_pcg_nextf(&GRandom);
}

int32 RandomRange(int32 Min, int32 Max)
{
	const int32 Range = Max - Min + 1;
	return (Range <= 0) ? Min : Min + (int32)(RandomNextF() * Range);
}

float32 RandomRangeF(float32 Min, float32 Max)
{
	const float32 Range = ABS(Max - Min);
	return MIN(Min, Max) + RandomNextF() * Range;
}