#pragma once

#include "Entity.h"
#include "Math2D.h"

#define BINARY_SEARCH_NAME(Type) CAT(BinarySearch_, Type)
#define BINARY_SEARCH_PROTOTYPE(type) int32 BINARY_SEARCH_NAME(type)(type Find, type * Data, int32 Count)

#define BINARY_SEARCH_INSERT_INDEX_NAME(Type) CAT(BinarySearchInsertIndex_, Type)
#define BINARY_SEARCH_INSERT_INDEX_PROTOTYPE(type)                                                                     \
	int32 BINARY_SEARCH_INSERT_INDEX_NAME(type)(type Find, type * Data, int32 Count)

#define BINARY_SEARCH_TOOLS_DEFINE_INTERFACE(type)                                                                     \
	BINARY_SEARCH_PROTOTYPE(type);                                                                                     \
	BINARY_SEARCH_INSERT_INDEX_PROTOTYPE(type);

#define BINARY_SEARCH_TYPES int8, int16, int32, int64, uint8, uint16, uint32, uint64, float32, float64, EntityId
FOR_EACH(BINARY_SEARCH_TOOLS_DEFINE_INTERFACE, BINARY_SEARCH_TYPES);

#define BINARY_SEARCH_GENERIC_ENTRY(Type) , Type : BINARY_SEARCH_NAME(Type)
#define BINARY_SEARCH_INSERT_INDEX_GENERIC_ENTRY(Type) , Type : BINARY_SEARCH_INSERT_INDEX_NAME(Type)

#define BinarySearch(Find, Data, Count)                                                                                \
	_Generic((Find)FOR_EACH(BINARY_SEARCH_GENERIC_ENTRY, BINARY_SEARCH_TYPES))(Find, Data, Count)

#define BinarySearchInsertIndex(Find, Data, Count)                                                                     \
	_Generic((Find)FOR_EACH(BINARY_SEARCH_INSERT_INDEX_GENERIC_ENTRY, BINARY_SEARCH_TYPES))(Find, Data, Count)

// Swap Functions
// -------------------------------------------------------
// clang-format off
#define SWAP(T, A, B) { T SWAP = A; A = B; B = SWAP; }
#define SWAP_REF(T, A, B) SWAP(T, *(A), *(B))

#define SWAP_NAME(Name) CAT(Swap, Name)
#define DEFINE_SWAP(T) static inline void CAT(Swap_, T)(T* A, T* B) { SWAP_REF(T, A, B); }
// clang-format on

// Any new types added to this will get a swap function defined for it and make it available via the Swap _Generic
#define SWAP_TYPES_LIST                                                                                                \
	bool, int8, int16, int32, int64, uint8, uint16, uint32, uint64, float32, float64, Vec2, Vec3, Vec4, Quat, Mat2,    \
		Mat3, Mat4, EntityId

FOR_EACH(DEFINE_SWAP, SWAP_TYPES_LIST);

#define SWAP_TYPE_GENERIC_ENTRY(T) , T : CAT(Swap_, T)
#define Swap(A, B) _Generic((A)FOR_EACH(SWAP_TYPE_GENERIC_ENTRY, SWAP_TYPES_LIST))(&A, &B)

#define SWAP_ELEMENTS(A, B, Size)                                                                                      \
	do {                                                                                                               \
		size_t SWAP_Size = (Size);                                                                                     \
		uint8* SWAP_A = (uint8*)(A);                                                                                   \
		uint8* SWAP_B = (uint8*)(B);                                                                                   \
		do {                                                                                                           \
			uint8 SWAP_Temp = *SWAP_A;                                                                                 \
			*SWAP_B++ = *SWAP_A;                                                                                       \
			*SWAP_A++ = SWAP_Temp;                                                                                     \
		} while (--SWAP_Size > 0);                                                                                     \
	} while (0);

static inline int8 TrueModuloInt8(int8 A, int8 B)
{
	return (A >= 0) ? A % B : ((B >= 0 ? B : -B) - 1 + (A + 1) % B);
}

static inline int16 TrueModuloInt16(int16 A, int16 B)
{
	return (A >= 0) ? A % B : ((B >= 0 ? B : -B) - 1 + (A + 1) % B);
}

static inline int32 TrueModuloInt32(int32 A, int32 B)
{
	return (A >= 0) ? A % B : ((B >= 0 ? B : -B) - 1 + (A + 1) % B);
}

static inline int64 TrueModuloInt64(int64 A, int64 B)
{
	return (A >= 0) ? A % B : ((B >= 0 ? B : -B) - 1 + (A + 1) % B);
}

static inline float32 TrueModuloFloat32(float32 A, float32 B)
{
	return A - B * floor(A / B);
}

static inline float64 TrueModuloFloat64(float64 A, float64 B)
{
	return A - B * floor(A / B);
}

#define TrueModulo(A, B)                                                                                               \
	_Generic(                                                                                                          \
		(A),                                                                                                           \
		int8: TrueModuloInt8,                                                                                          \
		int16: TrueModuloInt16,                                                                                        \
		int32: TrueModuloInt32,                                                                                        \
		int64: TrueModuloInt64,                                                                                        \
		float32: TrueModuloFloat32,                                                                                    \
		float64: TrueModuloFloat64)(A, B)

// Associative sorts provide a list of indices indicating what swaps occurs so they can be applied to arrays that have
// an associative relationship with the sorted array
void AssociativeInsertSort(
	void* Data,
	size_t ElementSize,
	size_t Count,
	int32* Indices,
	int32 (*Compare)(const void*, const void*));
void AssociativeQuickSort(
	void* Data,
	size_t ElementSize,
	size_t Count,
	int32* Indices,
	int32 (*Compare)(const void*, const void*));

void ApplyAssociativeIndices(void* Data, size_t ElementSize, size_t Count, int32* Indices);

#define SDL_zeroarr(arr) SDL_memset(arr, 0, arrlenu(arr) * sizeof(arr[0]))