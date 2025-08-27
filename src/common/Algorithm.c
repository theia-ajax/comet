#include "Algorithm.h"

#define BINARY_SEARCH_IMPL(type)                                                                                       \
	BINARY_SEARCH_PROTOTYPE(type)                                                                                      \
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

#define BINARY_SEARCH_INSERT_INDEX_IMPL(type)                                                                          \
	BINARY_SEARCH_INSERT_INDEX_PROTOTYPE(type)                                                                         \
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

#define BINARY_SEARCH_DEFAULT_IMPLEMENTATION(type)                                                                     \
	BINARY_SEARCH_IMPL(type)                                                                                           \
	BINARY_SEARCH_INSERT_INDEX_IMPL(type)

#define BINARY_SEARCH_DEFAULT_IMPL_TYPES int8, int16, int32, int64, uint8, uint16, uint32, uint64, float32, float64
FOR_EACH(BINARY_SEARCH_DEFAULT_IMPLEMENTATION, BINARY_SEARCH_DEFAULT_IMPL_TYPES)

BINARY_SEARCH_PROTOTYPE(EntityId)
{
	int32 Result = NONE;
	int32 Head = 0, Tail = Count, Mid = 0;
	while (Head < Tail) {
		Mid = (Tail - Head) / 2 + Head;
		if (Find.RawValue < Data[Mid].RawValue) {
			Tail = Mid;
		} else if (Find.RawValue > Data[Mid].RawValue) {
			Head = Mid + 1;
		} else {
			while (Mid > 0 && Data[Mid - 1].RawValue == Find.RawValue) {
				Mid--;
			}
			Result = Head = Tail = Mid;
			break;
		}
	}
	return Result;
}

BINARY_SEARCH_INSERT_INDEX_PROTOTYPE(EntityId)
{
	int32 Result = NONE;
	int32 Head = 0, Tail = Count, Mid = 0;
	while (Head < Tail) {
		Mid = (Tail - Head) / 2 + Head;
		if (Find.RawValue < Data[Mid].RawValue) {
			Tail = Mid;
		} else if (Find.RawValue > Data[Mid].RawValue) {
			Head = Mid + 1;
		} else {
			while (Mid > 0 && Data[Mid - 1].RawValue == Find.RawValue) {
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

// When number of elements in span is less than this fallback to insertion sort
#define QUICK_SORT_ELEMENTS_THRESHOLD 8

void AssociativeInsertSort(
	void* Data,
	size_t ElementSize,
	size_t Count,
	int32* Indices,
	int32 (*Compare)(const void*, const void*))
{
	ASSERT(Data);
	ASSERT(ElementSize > 0);

	if (Count == 0) {
		return;
	}

	uint8* Base = (uint8*)Data;
	for (int32 Index = 0; Index < Count; Index++) {
		Indices[Index] = Index;
	}

	for (int32 I = 1; I < Count; I++) {
		int32 J = I;
		while (J > 0 && Compare(Base + J * ElementSize, Base + (J - 1) * ElementSize) < 0) {
			SWAP_ELEMENTS(Base + J * ElementSize, Base + (J - 1) * ElementSize, ElementSize);
			SWAP_REF(int32, Indices + J, Indices + (J - 1));
		}
	}
}

typedef struct Span {
	uint8* Left;
	uint8* Right;
} Span;

#define QSORT_STACK_SIZE 8 * sizeof(size_t)
#define QSORT_STACK_PUSH(L, R) ((void)((Top->Left = (L)), (Top->Right = (R)), ++Top))
#define QSORT_STACK_POP(L, R) ((void)(--Top, (L = Top->Left), (R = Top->R)))

void AssociativeQuickSort(
	void* Data,
	size_t ElementSize,
	size_t Count,
	int32* Indices,
	int32 (*Compare)(const void*, const void*))
{
	uint8* Base = (uint8*)Data;

	Span Stack[QSORT_STACK_SIZE];
	Span* Top = Stack;

	while (Stack < Top) {
	}
}

void ApplyAssociativeIndices(void* Data, size_t ElementSize, size_t Count, int32* Indices)
{
	uint8* Base = (uint8*)Data;

	for (int32 Index = 0; Index < Count; Index++) {
		if (Indices[Index] != Index)
		{
			uint8* A = Base + Indices[Index] * ElementSize;
			uint8* B = Base + Index * ElementSize;
			SWAP_ELEMENTS(A, B, ElementSize);
		}
	}
}