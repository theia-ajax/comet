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

typedef float float32;
typedef double float64;

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

#define KILOBYTES(N) ((N) * 1024)
#define MEGABYTES(N) (KILOBYTES(N) * 1024)
#define GIGABYTES(N) (MEGABYTES(N) * 1024)
#define TERABYTES(N) (TERABYTES(N) * 1024)

#define NONE -1
#define CAT(x, y) CAT_(x, y)
#define CAT_(x, y) x##y

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
#define FOR_EACH(Expr, ...) __VA_OPT__(EXPAND(FOR_EACH_HELPER(Expr, __VA_ARGS__)))
#define FOR_EACH_HELPER(Expr, First, ...) Expr(First) __VA_OPT__(FOR_EACH_AGAIN PARENS(Expr, __VA_ARGS__))
#define FOR_EACH_AGAIN() FOR_EACH_HELPER

#define BIT_FLAG32(bit) (1u << (bit))
#define BIT_FLAG64(bit) (1llu << (bit))
#define MASK(index) (1 << (index))

#define ARRAY_COUNT(array) (sizeof((array)) / sizeof((array)[0]))
#define VALID_INDEX(index, count) (((index) >= 0) && ((index) < (count)))

#define ZERO(ptr, size) memset((ptr), 0, (size))

#define ZERO_STRUCT(struct_ptr) ZERO(struct_ptr, sizeof(*(struct_ptr)))
#define ZERO_ARRAY(array_ptr) ZERO(array_ptr, ARRAY_COUNT(array_ptr) * sizeof(*(array_ptr)))

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
#define FixedListAt(list, index) (&(list).Data[(index)])
#define FixedListLast(list) FixedListAt(list, (list).Count - 1)
#define FixedListIndexOf(list, item) ((item) - &(list).Data[0])
#define FixedListIsEmpty(list) ((list).Count == 0)
#define FixedListIsFull(list) ((list).Count == FixedListCapacity(list))
#define FixedListPush(list) &((list).Data[(list).Count++])
#define FixedListPop(list) (list).Count--
#define FixedListRemoveAt(list, index) (FixedListPop(list), (list).Data[index] = (list).Data[(list).Count])

#define HANDLE_INTERNAL_TYPE int32
#define DEFINE_HANDLE(Type)                                                                                            \
	typedef struct Type {                                                                                              \
		HANDLE_INTERNAL_TYPE Value;                                                                                    \
	} Type
#define VALID_HANDLE(Handle) ((Handle).Value != NONE)
typedef HANDLE_INTERNAL_TYPE handint;

#define UNIQUE_STATIC_NAME(x) CAT(CAT(x, _), __LINE__)
#define ONCE(x)                                                                                                        \
	{                                                                                                                  \
		static bool UNIQUE_STATIC_NAME(RunOnce) = false;                                                               \
		if (!UNIQUE_STATIC_NAME(RunOnce)) {                                                                            \
			UNIQUE_STATIC_NAME(RunOnce) = true;                                                                        \
			x;                                                                                                         \
		}                                                                                                              \
	}

// -------------------------------------------------------

SDL_NORETURN void PanicAndAbort(const char* Title, const char* Message);

typedef struct ColorU8 {
	uint8 R, G, B, A;
} ColorU8;
