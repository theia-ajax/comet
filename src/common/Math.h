#pragma once

#include "types.h"

#define HANDMADE_FLOAT real32
#define HANDMADE_MATH_USE_TURNS
#include <HandmadeMath.h>

#define KEpsilonFloat64 0.000000001
#define KEpsilonFloat32 0.00001f
#define KMaxFloat32 FLT_MAX
#define KMaxFloat64 DBL_MAX

static inline bool Approximately(real32 a, real32 b)
{
	return fabs(a - b) <= KEpsilonFloat32;
}

typedef struct Rect {
	int32 X, Y, W, H;
} Rect;

typedef struct Point {
	int32 X, Y;
} Point;

typedef struct Rect16 {
	int16 X, Y, W, H;
} Rect16;

typedef struct Point16 {
	int16 X, Y;
} Point16;

// clang-format off
#define SWAP(Type, A, B) do { Type SWAP = A; A = B; B = SWAP; } while (0)
#define SWAP_REF(Type, A, B) SWAP(Type, *A, *B)

#define SWAP_NAME(Name) NAME2(Swap, Name)
#define DEFINE_SWAP(Name, Type) static inline void SWAP_NAME(Name)(Type* A, Type* B) { SWAP_REF(Type, A, B); }
// clang-format on

DEFINE_SWAP(Int8, int8);
DEFINE_SWAP(Int16, int16);
DEFINE_SWAP(Int32, int32);
DEFINE_SWAP(Int64, int64);
DEFINE_SWAP(UInt8, uint8);
DEFINE_SWAP(UInt16, uint16);
DEFINE_SWAP(UInt32, uint32);
DEFINE_SWAP(UInt64, uint64);
DEFINE_SWAP(Float32, real32);
DEFINE_SWAP(Float64, real64);

#define Swap(A, B)                                                                                                     \
	_Generic(                                                                                                          \
		(A),                                                                                                           \
		int8: SwapInt8,                                                                                                \
		int16: SwapInt16,                                                                                              \
		int32: SwapInt32,                                                                                              \
		int64: SwapInt64,                                                                                              \
		uint8: SwapUInt8,                                                                                              \
		uint16: SwapUInt16,                                                                                            \
		uint32: SwapUInt32,                                                                                            \
		uint64: SwapUInt64,                                                                                            \
		real32: SwapFloat32,                                                                                           \
		real64: SwapFloat64)(&A, &B)
