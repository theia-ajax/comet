#pragma once

#include <SDL3/SDL_assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define ASSERT(expr) SDL_assert(expr)

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float flt32;
typedef double flt64;

#ifndef unreachable
#if defined(__GNUC__)
#define unreachable() (__builtin_unreachable())
#elif defined(_MSC_VER)
#define unreachable() (__assume(false))
#else
[[noreturn]] inline void unreachable_impl()
{
}
#define unreachable() (unreachable_impl())
#endif
#endif

#define NONE -1
#define CAT(x, y) CAT_(x, y)
#define CAT_(x, y) x ## y

#define EMPTY()
#define DEFER(id) id EMPTY()

#define COMMA() ,
#define CHAIN_COMMA_2(x) DEFER(COMMA)() x CHAIN_COMMA_1
#define CHAIN_COMMA_1(x) DEFER(COMMA)() x CHAIN_COMMA_2
#define CHAIN_COMMA_0(x) x CHAIN_COMMA_1
#define CHAIN_COMMA_0_END
#define CHAIN_COMMA_1_END
#define CHAIN_COMMA_2_END
#define CHAIN_COMMA(chain) CAT(CHAIN_COMMA_0 chain, _END)

#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define PARENS ()
#define FOR_EACH(Expr, ...) \
	__VA_OPT__(EXPAND(FOR_EACH_HELPER(Expr, __VA_ARGS__)))
#define FOR_EACH_HELPER(Expr, First, ...) \
	Expr(First) \
	__VA_OPT__(FOR_EACH_AGAIN PARENS (Expr, __VA_ARGS__))
#define FOR_EACH_AGAIN() FOR_EACH_HELPER

#define BIT_FLAG32(bit) (1u << (bit))
#define BIT_FLAG64(bit) (1llu << (bit))
#define MASK(index) (1 << (index))

#define ARRAY_COUNT(array) (sizeof((array)) / sizeof((array)[0]))
#define VALID_INDEX(index, count) (((index) >= 0) && ((index) < (count)))

#define ZERO(ptr, size) memset((ptr), 0, (size))

#define ZERO_STRUCT(struct_ptr) ZERO(struct_ptr, sizeof(*(struct_ptr)))
#define ZERO_ARRAY(array_ptr) ZERO(array_ptr, ARRAY_COUNT(array_ptr) * sizeof(*(array_ptr)))


#ifndef fixed_buffer
// fixed_buffer(name, size) creates a char array e.g. char name[size]
// as well as an accompanying const size_t name_size= size
#define fixed_buffer(name, size)                                                                                       \
	const size_t name##_size = size;                                                                                   \
	char name[size]
#endif

#ifndef static_fixed_buffer
#define static_fixed_buffer(name, size)                                                                                \
	static const size_t name##_size = size;                                                                            \
	static char name[size]
#endif

#define FixedArray(type, cap)                                                                                          \
	struct {                                                                                                           \
		type Data[cap];                                                                                                \
	}

#define FixedArrayCapacity(array) ARRAY_COUNT((array).Data)
#define FixedArrayElemSize(array) sizeof((array).Data[0])
#define FixedArraySize(array) sizeof((array).Data)

#define FixedList(type, cap)                                                                                           \
	struct {                                                                                                           \
		type Data[cap];                                                                                                \
		int32 Count;                                                                                                   \
	}

#define FixedListBegin(list) &((list).Data[0])
#define FixedListEnd(list) &((list).Data[(list).Count])
#define FixedListCapacity(list) ARRAY_COUNT((list).Data)
#define FixedListAt(list, index) &((list).Data[index])
#define FixedListLast(list) FixedListAt(list, (list).Count - 1)
#define FixedListIndexOf(list, item) ((item) - &(list).Data[0])
#define FixedListIsEmpty(list) ((list).Count == 0)
#define FixedListIsFull(list) ((list).Count == FixedListCapacity(list))
#define FixedListPush(list) &((list).Data[(list).Count++])
#define FixedListPop(list) (list).Count--
#define FixedListRemoveAt(list, index) (FixedListPop(list), (list).Data[index] = (list).Data[(list).Count])

// Swap Functions
// -------------------------------------------------------
// clang-format off
#ifdef SWAP
#undef SWAP
#endif

#define SWAP(T, A, B) { T SWAP = A; A = B; B = SWAP; }
#define SWAP_REF(T, A, B) SWAP(T, *A, *B)

#define SWAP_NAME(Name) CAT(Swap, Name)
#define DEFINE_SWAP(Name, T) static inline void SWAP_NAME(Name)(T* A, T* B) { SWAP_REF(T, A, B); }
// clang-format on

DEFINE_SWAP(Bool, bool);
DEFINE_SWAP(Int8, int8);
DEFINE_SWAP(Int16, int16);
DEFINE_SWAP(Int32, int32);
DEFINE_SWAP(Int64, int64);
DEFINE_SWAP(UInt8, uint8);
DEFINE_SWAP(UInt16, uint16);
DEFINE_SWAP(UInt32, uint32);
DEFINE_SWAP(UInt64, uint64);
DEFINE_SWAP(Float32, flt32);
DEFINE_SWAP(Float64, flt64);

#ifdef Swap
#warning                                                                                                               \
	"Swap already defined which means you probably don't need to manually include Types.h as it's being included from elsewhere."
#else
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
		flt32: SwapFloat32,                                                                                            \
		flt64: SwapFloat64)(A, B)
#endif

// Data Array
// -------------------------------------------------------
// $TODO: Revisit this entire data array thing and move it to it's own file if it's still going to be a thing
// -------------------------------------------------------
#define STRUCT(type) CAT(s_, type)
#define HANDLE(type) CAT(h_, type)
#define CONSTANT(type) CAT(k_, type)
#define CONSTANT_SUFFIX(type, suffix) CAT(CAT(k_, type), suffix)
#define DATA_ARRAY_NAME(type) CAT(type, _data_array)
#define DATA_ARRAY(type) STRUCT(DATA_ARRAY_NAME(type))

#define INVALID_HANDLE_VALUE 0
#define INVALID_HANDLE                                                                                                 \
	{                                                                                                                  \
		INVALID_HANDLE_VALUE                                                                                           \
	}

#define HANDLE_IS_VALID(handle) ((handle).value > INVALID_HANDLE_VALUE)
#define HANDLE_CREATE_FROM_INDEX(type, index)                                                                          \
	(HANDLE(type))                                                                                                     \
	{                                                                                                                  \
		(index) + 1                                                                                                    \
	}
#define HANDLE_INDEX(handle) ((handle).value - 1)
#define HANDLE_CONVERT_TO(new_type, handle) ((HANDLE(new_type)){(handle).value})

#define DECLARE_HANDLE(type)                                                                                           \
	typedef struct HANDLE(type) {                                                                                      \
		int32 value;                                                                                                   \
	} HANDLE(type)

#define DATA_ARRAY_FUNC_NAME(type, func) CAT(DATA_ARRAY_NAME(type), CAT(_, func))

#define DECLARE_DATA_ARRAY_GET(type)                                                                                   \
	STRUCT(type) * DATA_ARRAY_FUNC_NAME(type, get)(DATA_ARRAY(type) * array, HANDLE(type) handle)
#define DECLARE_DATA_ARRAY_TRY_GET(type)                                                                               \
	STRUCT(type)                                                                                                       \
	*DATA_ARRAY_FUNC_NAME(type, try_get)(DATA_ARRAY(type) * array, HANDLE(type) handle)
#define DECLARE_DATA_ARRAY_ALLOC(type) HANDLE(type) DATA_ARRAY_FUNC_NAME(type, alloc)(DATA_ARRAY(type) * array)

#define DECLARE_DATA_ARRAY_INTERFACE(type)                                                                             \
	DECLARE_DATA_ARRAY_ALLOC(type);                                                                                    \
	DECLARE_DATA_ARRAY_GET(type);                                                                                      \
	DECLARE_DATA_ARRAY_TRY_GET(type)

#define FORWARD_DECLARE_DATA_ARRAY(type) typedef struct DATA_ARRAY(type) DATA_ARRAY(type)

#define DATA_ARRAY_CAPACITY(type) CONSTANT_SUFFIX(type, _capacity)

#define DECLARE_DATA_ARRAY(type, capacity)                                                                             \
	DECLARE_HANDLE(type);                                                                                              \
	DECLARE_DATA_ARRAY_NO_HANDLE(type, capacity)

#define DECLARE_DATA_ARRAY_NO_HANDLE(type, capacity)                                                                   \
	enum { DATA_ARRAY_CAPACITY(type) = capacity };                                                                     \
	typedef struct DATA_ARRAY(type) {                                                                                  \
		STRUCT(type) data[DATA_ARRAY_CAPACITY(type) + 1];                                                              \
		int32 count;                                                                                                   \
	} DATA_ARRAY(type);                                                                                                \
	DECLARE_DATA_ARRAY_INTERFACE(type)

#define IMPLEMENT_DATA_ARRAY_GET(type)                                                                                 \
	DECLARE_DATA_ARRAY_GET(type)                                                                                       \
	{                                                                                                                  \
		ASSERT(HANDLE_IS_VALID(handle));                                                                               \
		ASSERT(VALID_INDEX(HANDLE_INDEX(handle), DATA_ARRAY_CAPACITY(type)));                                          \
		return array->data + HANDLE_INDEX(handle);                                                                     \
	}

#define IMPLEMENT_DATA_ARRAY_TRY_GET(type)                                                                             \
	DECLARE_DATA_ARRAY_TRY_GET(type)                                                                                   \
	{                                                                                                                  \
		STRUCT(type)* result = NULL;                                                                                   \
		if (VALID_INDEX(HANDLE_INDEX(handle), DATA_ARRAY_CAPACITY(type))) {                                            \
			result = array->data + HANDLE_INDEX(handle);                                                               \
		}                                                                                                              \
		return result;                                                                                                 \
	}

#define IMPLEMENT_DATA_ARRAY_ALLOC(type)                                                                               \
	DECLARE_DATA_ARRAY_ALLOC(type)                                                                                     \
	{                                                                                                                  \
		HANDLE(type) result = INVALID_HANDLE;                                                                          \
		ASSERT(array->count < DATA_ARRAY_CAPACITY(type));                                                              \
		result.value = array->count + 1;                                                                               \
		array->count++;                                                                                                \
		return result;                                                                                                 \
	}

#define IMPLEMENT_DATA_ARRAY_INTERFACE(type)                                                                           \
	IMPLEMENT_DATA_ARRAY_ALLOC(type)                                                                                   \
	IMPLEMENT_DATA_ARRAY_GET(type)                                                                                     \
	IMPLEMENT_DATA_ARRAY_TRY_GET(type)

#define IMPLEMENT_DATA_ARRAY(type) IMPLEMENT_DATA_ARRAY_INTERFACE(type)
// -------------------------------------------------------