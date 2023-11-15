#pragma once

#include "Entity.h"
#include "Math2D.h"

#define BINARY_SEARCH_DEFINE(type) type CAT(BinarySearch_, type)(type Find, type * Data, int32 Count)
#define BINARY_SEARCH_IMPL(type)                                                                                       \
	BINARY_SEARCH_DEFINE(type)                                                                                         \
	{                                                                                                                  \
		type Result = NONE;                                                                                            \
		int32 Head = 0, Tail = Count, Mid = 0;                                                                         \
		while (Head < Tail) {                                                                                          \
			Mid = (Tail - Head) / 2 + Head;                                                                            \
			if (Find < Data[Mid]) {                                                                                    \
				Tail = Mid;                                                                                            \
			} else if (Find > Data[Mid]) {                                                                             \
				Head = Mid + 1;                                                                                        \
			} else {                                                                                                   \
				while (Mid > 0 && Data[Mid - 1] == Find) {                                                             \
					Mid--;                                                                                             \
				}                                                                                                      \
				Result = Head = Tail = Mid;                                                                            \
				break;                                                                                                 \
			}                                                                                                          \
		}                                                                                                              \
		return Result;                                                                                                 \
	}

#define BINARY_SEARCH_INSERT_INDEX_DEFINE(type)                                                                        \
	int32 CAT(BinarySearchInsertIndex_, type)(int32 Find, int32 * Data, int32 Count)
#define BINARY_SEARCH_INSERT_INDEX_IMPL(type)                                                                          \
	BINARY_SEARCH_INSERT_INDEX_DEFINE(type)                                                                            \
	{                                                                                                                  \
		int32 Result = NONE;                                                                                           \
		int32 Head = 0, Tail = Count, Mid = 0;                                                                         \
		while (Head < Tail) {                                                                                          \
			Mid = (Tail - Head) / 2 + Head;                                                                            \
			if (Find < Data[Mid]) {                                                                                    \
				Tail = Mid;                                                                                            \
			} else if (Find > Data[Mid]) {                                                                             \
				Head = Mid + 1;                                                                                        \
			} else {                                                                                                   \
				while (Mid > 0 && Data[Mid - 1] == Find) {                                                             \
					Mid--;                                                                                             \
				}                                                                                                      \
				Result = Head = Tail = Mid;                                                                            \
				break;                                                                                                 \
			}                                                                                                          \
		}                                                                                                              \
		if (Result == NONE && Head == Tail) {                                                                          \
			Result = Head;                                                                                             \
		}                                                                                                              \
		return Result;                                                                                                 \
	}

#define BINARY_SEARCH_TOOLS_DEFINE(type)                                                                               \
	BINARY_SEARCH_DEFINE(type);                                                                                        \
	BINARY_SEARCH_INSERT_INDEX_DEFINE(type);
#define BINARY_SEARCH_TOOLS_IMPL(type)                                                                                 \
	BINARY_SEARCH_IMPL(type)                                                                                           \
	BINARY_SEARCH_INSERT_INDEX_IMPL(type)

BINARY_SEARCH_TOOLS_DEFINE(int8);
BINARY_SEARCH_TOOLS_DEFINE(int16);
BINARY_SEARCH_TOOLS_DEFINE(int32);
BINARY_SEARCH_TOOLS_DEFINE(int64);
BINARY_SEARCH_TOOLS_DEFINE(uint8);
BINARY_SEARCH_TOOLS_DEFINE(uint16);
BINARY_SEARCH_TOOLS_DEFINE(uint32);
BINARY_SEARCH_TOOLS_DEFINE(uint64);
BINARY_SEARCH_TOOLS_DEFINE(flt32);
BINARY_SEARCH_TOOLS_DEFINE(flt64);

#define BinarySearch(Find, Data, Count)                                                                                \
	_Generic(                                                                                                          \
		(Find),                                                                                                        \
		int8: BinarySearch_int8,                                                                                       \
		int16: BinarySearch_int16,                                                                                     \
		int32: BinarySearch_int32,                                                                                     \
		int64: BinarySearch_int64,                                                                                     \
		uint8: BinarySearch_uint8,                                                                                     \
		uint16: BinarySearch_uint16,                                                                                   \
		uint32: BinarySearch_uint32,                                                                                   \
		uint64: BinarySearch_uint64,                                                                                   \
		flt32: BinarySearch_flt32,                                                                                     \
		flt64: BinarySearch_flt64)(Find, Data, Count)

#define BinarySearchInsertIndex(Find, Data, Count)                                                                     \
	_Generic(                                                                                                          \
		(Find),                                                                                                        \
		int8: BinarySearchInsertIndex_int8,                                                                            \
		int16: BinarySearchInsertIndex_int16,                                                                          \
		int32: BinarySearchInsertIndex_int32,                                                                          \
		int64: BinarySearchInsertIndex_int64,                                                                          \
		uint8: BinarySearchInsertIndex_uint8,                                                                          \
		uint16: BinarySearchInsertIndex_uint16,                                                                        \
		uint32: BinarySearchInsertIndex_uint32,                                                                        \
		uint64: BinarySearchInsertIndex_uint64,                                                                        \
		flt32: BinarySearchInsertIndex_flt32,                                                                          \
		flt64: BinarySearchInsertIndex_flt64)(Find, Data, Count)

// Swap Functions
// -------------------------------------------------------
// clang-format off
#define SWAP(T, A, B) { T SWAP = A; A = B; B = SWAP; }
#define SWAP_REF(T, A, B) SWAP(T, *A, *B)

#define SWAP_NAME(Name) CAT(Swap, Name)
#define DEFINE_SWAP(T) static inline void CAT(Swap_, T)(T* A, T* B) { SWAP_REF(T, A, B); }
// clang-format on

// Any new types added to this will get a swap function defined for it and make it available via the Swap _Generic
#define SWAP_TYPES                                                                                                     \
	(bool)(int8)(int16)(int32)(int64)(uint8)(uint16)(uint32)(uint64) \
	(flt32)(flt64)(Vec2)(Vec3)(Vec4)(Quat)(Mat2)(Mat3)(Mat4)(EntityId)
#define SWAP_TYPES_LIST CHAIN_COMMA(SWAP_TYPES)

FOR_EACH(DEFINE_SWAP, SWAP_TYPES_LIST);

#define SWAP_TYPE_GENERIC_ENTRY(T) , T : CAT(Swap_, T)
#define Swap(A, B) _Generic((A)FOR_EACH(SWAP_TYPE_GENERIC_ENTRY, SWAP_TYPES_LIST))(&A, &B)