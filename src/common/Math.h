#pragma once

#include "types.h"

#define HANDMADE_FLOAT real32
#define HANDMADE_MATH_USE_TURNS
#include <HandmadeMath.h>

#define KEpsilon 0.000000001
#define KEpsilon32 0.00001f

static inline bool Approximately(real32 a, real32 b)
{
	return fabs(a - b) <= KEpsilon32;
}
