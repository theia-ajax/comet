/*
  HandmadeMath.h v2.0.0

  tedajax fork:

  My Edits:
    - Remove HMM_ prefix, adjust some variable names accordingly
    - Parameters reordered for Clamp from min, value, max to value, min, max
    - Parameters reordered for Lerp from a, t, b to a, b, t
    - Extend clamp to ClampV2/V3/V4 as component-wise clamp
    - Constants from ALLCAPS style to KPascalCase style
    - Updated all tests to work with changes
    - Scale -> ScaleV3
    - Translate -> TranslateV3
    - Added Abs/Min/Max Scalar and Vectors ops, keeping old preprocessors ones
    - Added IsFinite F/V2/V3/V4
    - Changed field name 'Elements' -> 'Data'

  This is a single header file with a bunch of useful types and functions for
  games and graphics. Consider it a lightweight alternative to GLM that works
  both C and C++.

  =============================================================================
  CONFIG
  =============================================================================

  By default, all angles in Handmade Math are specified in radians. However, it
  can be configured to use degrees or turns instead. Use one of the following
  defines to specify the default unit for angles:

    #define HANDMADE_MATH_USE_RADIANS
    #define HANDMADE_MATH_USE_DEGREES
    #define HANDMADE_MATH_USE_TURNS

  Regardless of the default angle, you can use the following functions to
  specify an angle in a particular unit:

    AngleRad(radians)
    AngleDeg(degrees)
    AngleTurn(turns)

  The definitions of these functions change depending on the default unit.

  -----------------------------------------------------------------------------

  Handmade Math ships with SSE (SIMD) implementations of several common
  operations. To disable the use of SSE intrinsics, you must define
  HANDMADE_MATH_NO_SSE before including this file:

    #define HANDMADE_MATH_NO_SSE
    #include "HandmadeMath.h"

  -----------------------------------------------------------------------------

  To use Handmade Math without the C runtime library, you must provide your own
  implementations of basic math functions. Otherwise, HandmadeMath.h will use
  the runtime library implementation of these functions.

  Define HANDMADE_MATH_PROVIDE_MATH_FUNCTIONS and provide your own
  implementations of SINF, COSF, TANF, ACOSF, and SQRTF
  before including HandmadeMath.h, like so:

    #define HANDMADE_MATH_PROVIDE_MATH_FUNCTIONS
    #define SINF MySinF
    #define COSF MyCosF
    #define TANF MyTanF
    #define ACOSF MyACosF
    #define SQRTF MySqrtF
    #define ISFINITE MyIsFinite
    #include "HandmadeMath.h"

  By default, it is assumed that your math functions take radians. To use
  different units, you must define ANGLE_USER_TO_INTERNAL and
  ANGLE_INTERNAL_TO_USER. For example, if you want to use degrees in your
  code but your math functions use turns:

    #define ANGLE_USER_TO_INTERNAL(a) ((a)*DegToTurn)
    #define ANGLE_INTERNAL_TO_USER(a) ((a)*TurnToDeg)

  =============================================================================

  LICENSE

  This software is in the public domain. Where that dedication is not
  recognized, you are granted a perpetual, irrevocable license to copy,
  distribute, and modify this file as you see fit.

  =============================================================================

  CREDITS

  Originally written by Zakary Strange.

  Functionality:
   Zakary Strange (strangezak@protonmail.com && @strangezak)
   Matt Mascarenhas (@miblo_)
   Aleph
   FieryDrake (@fierydrake)
   Gingerbill (@TheGingerBill)
   Ben Visness (@bvisness)
   Trinton Bullard (@Peliex_Dev)
   @AntonDan
   Logan Forman (@dev_dwarf)

  Fixes:
   Jeroen van Rijn (@J_vanRijn)
   Kiljacken (@Kiljacken)
   Insofaras (@insofaras)
   Daniel Gibson (@DanielGibson)
*/

#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

// Dummy macros for when test framework is not present.
#ifndef COVERAGE
#define COVERAGE(a, b)
#endif

#ifndef ASSERT_COVERED
#define ASSERT_COVERED(a)
#endif

#ifndef HANDMADE_FLOAT
#define Float float
#else
#define Float HANDMADE_FLOAT
#endif

#ifndef HANDMADE_BOOL
#include <stdbool.h>
#define Bool bool
#else
#define Bool char
#endif

/* let's figure out if SSE is really available (unless disabled anyway)
   (it isn't on non-x86/x86_64 platforms or even x86 without explicit SSE support)
   => only use "#ifdef HANDMADE_MATH__USE_SSE" to check for SSE support below this block! */
#ifndef HANDMADE_MATH_NO_SSE
#ifdef _MSC_VER /* MSVC supports SSE in amd64 mode or _M_IX86_FP >= 1 (2 means SSE2) */
#if defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#define HANDMADE_MATH__USE_SSE 1
#endif
#else          /* not MSVC, probably GCC, clang, icc or something that doesn't support SSE anyway */
#ifdef __SSE__ /* they #define __SSE__ if it's supported */
#define HANDMADE_MATH__USE_SSE 1
#endif /*  __SSE__ */
#endif /* not _MSC_VER */
#endif /* #ifndef HANDMADE_MATH_NO_SSE */

#if (!defined(__cplusplus) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
#define HANDMADE_MATH__USE_C11_GENERICS 1
#endif

#ifdef HANDMADE_MATH__USE_SSE
#include <xmmintrin.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4201)
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#if (defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ < 8)) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wgnu-anonymous-struct"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#define DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#define DEPRECATED(msg) __declspec(deprecated(msg))
#else
#define DEPRECATED(msg)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(HANDMADE_MATH_USE_DEGREES) && !defined(HANDMADE_MATH_USE_TURNS) && !defined(HANDMADE_MATH_USE_RADIANS)
#define HANDMADE_MATH_USE_RADIANS
#endif

#define KPi 3.14159265358979323846
#define KPi32 3.14159265359f
#define KTau 6.28318530717958647692
#define KTau32 6.28318530718f
#define KDeg180 180.0
#define KDeg18032 180.0f
#define KDeg360 360.0
#define KDeg36032 360.0f
#define KTurnHalf 0.5
#define KTurnHalf32 0.5f
#define KTurnFull 1.0
#define KTurnFull32 1.0f
#define RadToDeg ((Float)(KDeg180 / KPi))
#define RadToTurn ((Float)(KTurnHalf / KPi))
#define DegToRad ((Float)(KPi / KDeg180))
#define DegToTurn ((Float)(KTurnHalf / KDeg180))
#define TurnToRad ((Float)(KPi / KTurnHalf))
#define TurnToDeg ((Float)(KDeg180 / KTurnHalf))

#if defined(HANDMADE_MATH_USE_RADIANS)
#define AngleRad(a) (a)
#define AngleDeg(a) ((a) * DegToRad)
#define AngleTurn(a) ((a) * TurnToRad)
#define KUnitFullTurn KTau
#define KUnitFullTurn32 KTau32
#elif defined(HANDMADE_MATH_USE_DEGREES)
#define AngleRad(a) ((a) * RadToDeg)
#define AngleDeg(a) (a)
#define AngleTurn(a) ((a) * TurnToDeg)
#define KUnitFullTurn KDeg360
#define KUnitFullTurn32 KDeg36032
#elif defined(HANDMADE_MATH_USE_TURNS)
#define AngleRad(a) ((a) * RadToTurn)
#define AngleDeg(a) ((a) * DegToTurn)
#define AngleTurn(a) (a)
#define KUnitFullTurn KTurnFull
#define KUnitFullTurn32 KTurnFull32
#endif

#if !defined(HANDMADE_MATH_PROVIDE_MATH_FUNCTIONS)
#include <math.h>
#define SINF sinf
#define COSF cosf
#define TANF tanf
#define SQRTF sqrtf
#define ACOSF acosf
#define ISFINITE isfinite
#endif

#if !defined(ANGLE_USER_TO_INTERNAL)
#define ANGLE_USER_TO_INTERNAL(a) (ToRad(a))
#endif

#if !defined(ANGLE_INTERNAL_TO_USER)
#if defined(HANDMADE_MATH_USE_RADIANS)
#define ANGLE_INTERNAL_TO_USER(a) (a)
#elif defined(HANDMADE_MATH_USE_DEGREES)
#define ANGLE_INTERNAL_TO_USER(a) ((a) * RadToDeg)
#elif defined(HANDMADE_MATH_USE_TURNS)
#define ANGLE_INTERNAL_TO_USER(a) ((a) * RadToTurn)
#endif
#endif

// MIN, MAX, ABS have function equivalents to match V2/3/4 versions
// Keeping the defines around for use on integer types
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define ABS(a) ((a) > 0 ? (a) : -(a))
#define MOD(a, m) (((a) % (m)) >= 0 ? ((a) % (m)) : (((a) % (m)) + (m)))
#define SQUARE(x) ((x) * (x))

    typedef union Vec2
    {
        struct
        {
            Float X, Y;
        };

        struct
        {
            Float U, V;
        };

        struct
        {
            Float Left, Right;
        };

        struct
        {
            Float Width, Height;
        };

        Float Data[2];

#ifdef __cplusplus
        inline Float &operator[](int Index)
        {
            return Data[Index];
        }
#endif
    } Vec2;

    typedef union Vec3
    {
        struct
        {
            Float X, Y, Z;
        };

        struct
        {
            Float U, V, W;
        };

        struct
        {
            Float R, G, B;
        };

        struct
        {
            Vec2 XY;
            Float _Ignored0;
        };

        struct
        {
            Float _Ignored1;
            Vec2 YZ;
        };

        struct
        {
            Vec2 UV;
            Float _Ignored2;
        };

        struct
        {
            Float _Ignored3;
            Vec2 VW;
        };

        Float Data[3];

#ifdef __cplusplus
        inline Float &operator[](int Index)
        {
            return Data[Index];
        }
#endif
    } Vec3;

    typedef union Vec4
    {
        struct
        {
            union
            {
                Vec3 XYZ;
                struct
                {
                    Float X, Y, Z;
                };
            };

            Float W;
        };
        struct
        {
            union
            {
                Vec3 RGB;
                struct
                {
                    Float R, G, B;
                };
            };

            Float A;
        };

        struct
        {
            Vec2 XY;
            Float _Ignored0;
            Float _Ignored1;
        };

        struct
        {
            Float _Ignored2;
            Vec2 YZ;
            Float _Ignored3;
        };

        struct
        {
            Float _Ignored4;
            Float _Ignored5;
            Vec2 ZW;
        };

        Float Data[4];

#ifdef HANDMADE_MATH__USE_SSE
        __m128 SSE;
#endif

#ifdef __cplusplus
        inline Float &operator[](int Index)
        {
            return Data[Index];
        }
#endif
    } Vec4;

    typedef union Mat2
    {
        Float Data[2][2];
        Vec2 Columns[2];

#ifdef __cplusplus
        inline Vec2 &operator[](int Index)
        {
            return Columns[Index];
        }
#endif
    } Mat2;

    typedef union Mat3
    {
        Float Data[3][3];
        Vec3 Columns[3];

#ifdef __cplusplus
        inline Vec3 &operator[](int Index)
        {
            return Columns[Index];
        }
#endif
    } Mat3;

    typedef union Mat4
    {
        Float Data[4][4];
        Vec4 Columns[4];

#ifdef __cplusplus
        inline Vec4 &operator[](int Index)
        {
            return Columns[Index];
        }
#endif
    } Mat4;

    typedef union Quat
    {
        struct
        {
            union
            {
                Vec3 XYZ;
                struct
                {
                    Float X, Y, Z;
                };
            };

            Float W;
        };

        Float Data[4];

#ifdef HANDMADE_MATH__USE_SSE
        __m128 SSE;
#endif
    } Quat;

    /*
     * Angle unit conversion functions
     */
    static inline Float ToRad(Float Angle)
    {
#if defined(HANDMADE_MATH_USE_RADIANS)
        Float Result = Angle;
#elif defined(HANDMADE_MATH_USE_DEGREES)
    Float Result = Angle * DegToRad;
#elif defined(HANDMADE_MATH_USE_TURNS)
    Float Result = Angle * TurnToRad;
#endif

        return Result;
    }

    static inline Float ToDeg(Float Angle)
    {
#if defined(HANDMADE_MATH_USE_RADIANS)
        Float Result = Angle * RadToDeg;
#elif defined(HANDMADE_MATH_USE_DEGREES)
    Float Result = Angle;
#elif defined(HANDMADE_MATH_USE_TURNS)
    Float Result = Angle * TurnToDeg;
#endif

        return Result;
    }

    static inline Float ToTurn(Float Angle)
    {
#if defined(HANDMADE_MATH_USE_RADIANS)
        Float Result = Angle * RadToTurn;
#elif defined(HANDMADE_MATH_USE_DEGREES)
    Float Result = Angle * DegToTurn;
#elif defined(HANDMADE_MATH_USE_TURNS)
    Float Result = Angle;
#endif

        return Result;
    }

    /*
     * Floating-point math functions
     */

    COVERAGE(SinF, 1)
    static inline Float SinF(Float Angle)
    {
        ASSERT_COVERED(SinF);
        return SINF(ANGLE_USER_TO_INTERNAL(Angle));
    }

    COVERAGE(CosF, 1)
    static inline Float CosF(Float Angle)
    {
        ASSERT_COVERED(CosF);
        return COSF(ANGLE_USER_TO_INTERNAL(Angle));
    }

    COVERAGE(TanF, 1)
    static inline Float TanF(Float Angle)
    {
        ASSERT_COVERED(TanF);
        return TANF(ANGLE_USER_TO_INTERNAL(Angle));
    }

    COVERAGE(ACosF, 1)
    static inline Float ACosF(Float Arg)
    {
        ASSERT_COVERED(ACosF);
        return ANGLE_INTERNAL_TO_USER(ACOSF(Arg));
    }

    COVERAGE(SqrtF, 1)
    static inline Float SqrtF(Float Value)
    {
        ASSERT_COVERED(SqrtF);

        Float Result;

#ifdef HANDMADE_MATH__USE_SSE
        __m128 In = _mm_set_ss(Value);
        __m128 Out = _mm_sqrt_ss(In);
        Result = _mm_cvtss_f32(Out);
#else
    Result = SQRTF(Value);
#endif

        return Result;
    }

    COVERAGE(InvSqrtF, 1)
    static inline Float InvSqrtF(Float Value)
    {
        ASSERT_COVERED(InvSqrtF);

        Float Result;

        Result = 1.0f / SqrtF(Value);

        return Result;
    }

    /*
     * Utility functions
     */

    COVERAGE(Min, 1)
    static inline Float Min(Float A, Float B)
    {
        ASSERT_COVERED(Min);
        return (A < B) ? A : B;
    }

    COVERAGE(Max, 1)
    static inline Float Max(Float A, Float B)
    {
        ASSERT_COVERED(Max);
        return (A < B) ? B : A;
    }

    COVERAGE(Abs, 1)
    static inline Float Abs(Float A)
    {
        ASSERT_COVERED(Abs);
        return (A >= 0) ? A : -A;
    }

    COVERAGE(Lerp, 1)
    static inline Float Lerp(Float A, Float B, Float Time)
    {
        ASSERT_COVERED(Lerp);
        return (1.0f - Time) * A + Time * B;
    }

    COVERAGE(Clamp, 1)
    static inline Float Clamp(Float Value, Float MinValue, Float MaxValue)
    {
        ASSERT_COVERED(Clamp);
        return (Value < MinValue) ? MinValue : ((Value > MaxValue) ? MaxValue : Value);
    }

    COVERAGE(IsFinite, 1)
    static inline Bool IsFinite(Float Value)
    {
        ASSERT_COVERED(IsFinite)
        return ISFINITE(Value);
    }

    /*
     * Vector initialization
     */

    COVERAGE(V2, 1)
    static inline Vec2 V2(Float X, Float Y)
    {
        ASSERT_COVERED(V2);

        Vec2 Result;
        Result.X = X;
        Result.Y = Y;

        return Result;
    }

    COVERAGE(V3, 1)
    static inline Vec3 V3(Float X, Float Y, Float Z)
    {
        ASSERT_COVERED(V3);

        Vec3 Result;
        Result.X = X;
        Result.Y = Y;
        Result.Z = Z;

        return Result;
    }

    COVERAGE(V4, 1)
    static inline Vec4 V4(Float X, Float Y, Float Z, Float W)
    {
        ASSERT_COVERED(V4);

        Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
        Result.SSE = _mm_setr_ps(X, Y, Z, W);
#else
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
    Result.W = W;
#endif

        return Result;
    }

    COVERAGE(V4V, 1)
    static inline Vec4 V4V(Vec3 Vector, Float W)
    {
        ASSERT_COVERED(V4V);

        Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
        Result.SSE = _mm_setr_ps(Vector.X, Vector.Y, Vector.Z, W);
#else
    Result.XYZ = Vector;
    Result.W = W;
#endif

        return Result;
    }

    /*
     * Binary vector operations
     */

    COVERAGE(AddV2, 1)
    static inline Vec2 AddV2(Vec2 Left, Vec2 Right)
    {
        ASSERT_COVERED(AddV2);

        Vec2 Result;
        Result.X = Left.X + Right.X;
        Result.Y = Left.Y + Right.Y;

        return Result;
    }

    COVERAGE(AddV3, 1)
    static inline Vec3 AddV3(Vec3 Left, Vec3 Right)
    {
        ASSERT_COVERED(AddV3);

        Vec3 Result;
        Result.X = Left.X + Right.X;
        Result.Y = Left.Y + Right.Y;
        Result.Z = Left.Z + Right.Z;

        return Result;
    }

    COVERAGE(AddV4, 1)
    static inline Vec4 AddV4(Vec4 Left, Vec4 Right)
    {
        ASSERT_COVERED(AddV4);

        Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
        Result.SSE = _mm_add_ps(Left.SSE, Right.SSE);
#else
    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    Result.Z = Left.Z + Right.Z;
    Result.W = Left.W + Right.W;
#endif

        return Result;
    }

    COVERAGE(SubV2, 1)
    static inline Vec2 SubV2(Vec2 Left, Vec2 Right)
    {
        ASSERT_COVERED(SubV2);

        Vec2 Result;
        Result.X = Left.X - Right.X;
        Result.Y = Left.Y - Right.Y;

        return Result;
    }

    COVERAGE(SubV3, 1)
    static inline Vec3 SubV3(Vec3 Left, Vec3 Right)
    {
        ASSERT_COVERED(SubV3);

        Vec3 Result;
        Result.X = Left.X - Right.X;
        Result.Y = Left.Y - Right.Y;
        Result.Z = Left.Z - Right.Z;

        return Result;
    }

    COVERAGE(SubV4, 1)
    static inline Vec4 SubV4(Vec4 Left, Vec4 Right)
    {
        ASSERT_COVERED(SubV4);

        Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
        Result.SSE = _mm_sub_ps(Left.SSE, Right.SSE);
#else
    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    Result.Z = Left.Z - Right.Z;
    Result.W = Left.W - Right.W;
#endif

        return Result;
    }

    COVERAGE(MulV2, 1)
    static inline Vec2 MulV2(Vec2 Left, Vec2 Right)
    {
        ASSERT_COVERED(MulV2);

        Vec2 Result;
        Result.X = Left.X * Right.X;
        Result.Y = Left.Y * Right.Y;

        return Result;
    }

    COVERAGE(MulV2F, 1)
    static inline Vec2 MulV2F(Vec2 Left, Float Right)
    {
        ASSERT_COVERED(MulV2F);

        Vec2 Result;
        Result.X = Left.X * Right;
        Result.Y = Left.Y * Right;

        return Result;
    }

    COVERAGE(MulV3, 1)
    static inline Vec3 MulV3(Vec3 Left, Vec3 Right)
    {
        ASSERT_COVERED(MulV3);

        Vec3 Result;
        Result.X = Left.X * Right.X;
        Result.Y = Left.Y * Right.Y;
        Result.Z = Left.Z * Right.Z;

        return Result;
    }

    COVERAGE(MulV3F, 1)
    static inline Vec3 MulV3F(Vec3 Left, Float Right)
    {
        ASSERT_COVERED(MulV3F);

        Vec3 Result;
        Result.X = Left.X * Right;
        Result.Y = Left.Y * Right;
        Result.Z = Left.Z * Right;

        return Result;
    }

    COVERAGE(MulV4, 1)
    static inline Vec4 MulV4(Vec4 Left, Vec4 Right)
    {
        ASSERT_COVERED(MulV4);

        Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
        Result.SSE = _mm_mul_ps(Left.SSE, Right.SSE);
#else
    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;
    Result.Z = Left.Z * Right.Z;
    Result.W = Left.W * Right.W;
#endif

        return Result;
    }

    COVERAGE(MulV4F, 1)
    static inline Vec4 MulV4F(Vec4 Left, Float Right)
    {
        ASSERT_COVERED(MulV4F);

        Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
        __m128 Scalar = _mm_set1_ps(Right);
        Result.SSE = _mm_mul_ps(Left.SSE, Scalar);
#else
    Result.X = Left.X * Right;
    Result.Y = Left.Y * Right;
    Result.Z = Left.Z * Right;
    Result.W = Left.W * Right;
#endif

        return Result;
    }

    COVERAGE(DivV2, 1)
    static inline Vec2 DivV2(Vec2 Left, Vec2 Right)
    {
        ASSERT_COVERED(DivV2);

        Vec2 Result;
        Result.X = Left.X / Right.X;
        Result.Y = Left.Y / Right.Y;

        return Result;
    }

    COVERAGE(DivV2F, 1)
    static inline Vec2 DivV2F(Vec2 Left, Float Right)
    {
        ASSERT_COVERED(DivV2F);

        Vec2 Result;
        Result.X = Left.X / Right;
        Result.Y = Left.Y / Right;

        return Result;
    }

    COVERAGE(DivV3, 1)
    static inline Vec3 DivV3(Vec3 Left, Vec3 Right)
    {
        ASSERT_COVERED(DivV3);

        Vec3 Result;
        Result.X = Left.X / Right.X;
        Result.Y = Left.Y / Right.Y;
        Result.Z = Left.Z / Right.Z;

        return Result;
    }

    COVERAGE(DivV3F, 1)
    static inline Vec3 DivV3F(Vec3 Left, Float Right)
    {
        ASSERT_COVERED(DivV3F);

        Vec3 Result;
        Result.X = Left.X / Right;
        Result.Y = Left.Y / Right;
        Result.Z = Left.Z / Right;

        return Result;
    }

    COVERAGE(DivV4, 1)
    static inline Vec4 DivV4(Vec4 Left, Vec4 Right)
    {
        ASSERT_COVERED(DivV4);

        Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
        Result.SSE = _mm_div_ps(Left.SSE, Right.SSE);
#else
    Result.X = Left.X / Right.X;
    Result.Y = Left.Y / Right.Y;
    Result.Z = Left.Z / Right.Z;
    Result.W = Left.W / Right.W;
#endif

        return Result;
    }

    COVERAGE(DivV4F, 1)
    static inline Vec4 DivV4F(Vec4 Left, Float Right)
    {
        ASSERT_COVERED(DivV4F);

        Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
        __m128 Scalar = _mm_set1_ps(Right);
        Result.SSE = _mm_div_ps(Left.SSE, Scalar);
#else
    Result.X = Left.X / Right;
    Result.Y = Left.Y / Right;
    Result.Z = Left.Z / Right;
    Result.W = Left.W / Right;
#endif

        return Result;
    }

    COVERAGE(EqV2, 1)
    static inline Bool EqV2(Vec2 Left, Vec2 Right)
    {
        ASSERT_COVERED(EqV2);
        return Left.X == Right.X && Left.Y == Right.Y;
    }

    COVERAGE(EqV3, 1)
    static inline Bool EqV3(Vec3 Left, Vec3 Right)
    {
        ASSERT_COVERED(EqV3);
        return Left.X == Right.X && Left.Y == Right.Y && Left.Z == Right.Z;
    }

    COVERAGE(EqV4, 1)
    static inline Bool EqV4(Vec4 Left, Vec4 Right)
    {
        ASSERT_COVERED(EqV4);
        return Left.X == Right.X && Left.Y == Right.Y && Left.Z == Right.Z && Left.W == Right.W;
    }

    COVERAGE(DotV2, 1)
    static inline Float DotV2(Vec2 Left, Vec2 Right)
    {
        ASSERT_COVERED(DotV2);
        return (Left.X * Right.X) + (Left.Y * Right.Y);
    }

    COVERAGE(DotV3, 1)
    static inline Float DotV3(Vec3 Left, Vec3 Right)
    {
        ASSERT_COVERED(DotV3);
        return (Left.X * Right.X) + (Left.Y * Right.Y) + (Left.Z * Right.Z);
    }

    COVERAGE(DotV4, 1)
    static inline Float DotV4(Vec4 Left, Vec4 Right)
    {
        ASSERT_COVERED(DotV4);

        Float Result;

        // NOTE(zak): IN the future if we wanna check what version SSE is support
        // we can use _mm_dp_ps (4.3) but for now we will use the old way.
        // Or a r = _mm_mul_ps(v1, v2), r = _mm_hadd_ps(r, r), r = _mm_hadd_ps(r, r) for SSE3
#ifdef HANDMADE_MATH__USE_SSE
        __m128 SSEResultOne = _mm_mul_ps(Left.SSE, Right.SSE);
        __m128 SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
        SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
        SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
        SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
        _mm_store_ss(&Result, SSEResultOne);
#else
    Result = ((Left.X * Right.X) + (Left.Z * Right.Z)) + ((Left.Y * Right.Y) + (Left.W * Right.W));
#endif

        return Result;
    }

    COVERAGE(Cross, 1)
    static inline Vec3 Cross(Vec3 Left, Vec3 Right)
    {
        ASSERT_COVERED(Cross);

        Vec3 Result;
        Result.X = (Left.Y * Right.Z) - (Left.Z * Right.Y);
        Result.Y = (Left.Z * Right.X) - (Left.X * Right.Z);
        Result.Z = (Left.X * Right.Y) - (Left.Y * Right.X);

        return Result;
    }

    /*
     * Unary vector operations
     */

    COVERAGE(LenSqrV2, 1)
    static inline Float LenSqrV2(Vec2 A)
    {
        ASSERT_COVERED(LenSqrV2);
        return DotV2(A, A);
    }

    COVERAGE(LenSqrV3, 1)
    static inline Float LenSqrV3(Vec3 A)
    {
        ASSERT_COVERED(LenSqrV3);
        return DotV3(A, A);
    }

    COVERAGE(LenSqrV4, 1)
    static inline Float LenSqrV4(Vec4 A)
    {
        ASSERT_COVERED(LenSqrV4);
        return DotV4(A, A);
    }

    COVERAGE(LenV2, 1)
    static inline Float LenV2(Vec2 A)
    {
        ASSERT_COVERED(LenV2);
        return SqrtF(LenSqrV2(A));
    }

    COVERAGE(LenV3, 1)
    static inline Float LenV3(Vec3 A)
    {
        ASSERT_COVERED(LenV3);
        return SqrtF(LenSqrV3(A));
    }

    COVERAGE(LenV4, 1)
    static inline Float LenV4(Vec4 A)
    {
        ASSERT_COVERED(LenV4);
        return SqrtF(LenSqrV4(A));
    }

    COVERAGE(NormV2, 1)
    static inline Vec2 NormV2(Vec2 A)
    {
        ASSERT_COVERED(NormV2);
        return MulV2F(A, InvSqrtF(DotV2(A, A)));
    }

    COVERAGE(NormV3, 1)
    static inline Vec3 NormV3(Vec3 A)
    {
        ASSERT_COVERED(NormV3);
        return MulV3F(A, InvSqrtF(DotV3(A, A)));
    }

    COVERAGE(NormV4, 1)
    static inline Vec4 NormV4(Vec4 A)
    {
        ASSERT_COVERED(NormV4);
        return MulV4F(A, InvSqrtF(DotV4(A, A)));
    }

    /*
     * Utility vector functions
     */

    COVERAGE(MinV2, 1)
    static inline Vec2 MinV2(Vec2 A, Vec2 B)
    {
        ASSERT_COVERED(MinV2);
        return V2(Min(A.X, B.X), Min(A.Y, B.Y));
    }

    COVERAGE(MinV3, 1)
    static inline Vec3 MinV3(Vec3 A, Vec3 B)
    {
        ASSERT_COVERED(MinV3);
        return V3(Min(A.X, B.X), Min(A.Y, B.Y), Min(A.Z, B.Z));
    }

    COVERAGE(MinV4, 1)
    static inline Vec4 MinV4(Vec4 A, Vec4 B)
    {
        ASSERT_COVERED(MinV4);
        return V4(Min(A.X, B.X), Min(A.Y, B.Y), Min(A.Z, B.Z), Min(A.W, B.W));
    }

    COVERAGE(MaxV2, 1)
    static inline Vec2 MaxV2(Vec2 A, Vec2 B)
    {
        ASSERT_COVERED(MaxV2);
        return V2(Max(A.X, B.X), Max(A.Y, B.Y));
    }

    COVERAGE(MaxV3, 1)
    static inline Vec3 MaxV3(Vec3 A, Vec3 B)
    {
        ASSERT_COVERED(MaxV3);
        return V3(Max(A.X, B.X), Max(A.Y, B.Y), Max(A.Z, B.Z));
    }

    COVERAGE(MaxV4, 1)
    static inline Vec4 MaxV4(Vec4 A, Vec4 B)
    {
        ASSERT_COVERED(MaxV4);
        return V4(Max(A.X, B.X), Max(A.Y, B.Y), Max(A.Z, B.Z), Max(A.W, B.W));
    }

    COVERAGE(AbsV2, 1)
    static inline Vec2 AbsV2(Vec2 V)
    {
        ASSERT_COVERED(AbsV2);
        return V2(Abs(V.X), Abs(V.Y));
    }

    COVERAGE(AbsV3, 1)
    static inline Vec3 AbsV3(Vec3 V)
    {
        ASSERT_COVERED(AbsV3);
        return V3(Abs(V.X), Abs(V.Y), Abs(V.Z));
    }

    COVERAGE(AbsV4, 1)
    static inline Vec4 AbsV4(Vec4 V)
    {
        ASSERT_COVERED(AbsV4);
        return V4(Abs(V.X), Abs(V.Y), Abs(V.Z), Abs(V.W));
    }

    COVERAGE(LerpV2, 1)
    static inline Vec2 LerpV2(Vec2 A, Vec2 B, Float Time)
    {
        ASSERT_COVERED(LerpV2);
        return AddV2(MulV2F(A, 1.0f - Time), MulV2F(B, Time));
    }

    COVERAGE(LerpV3, 1)
    static inline Vec3 LerpV3(Vec3 A, Vec3 B, Float Time)
    {
        ASSERT_COVERED(LerpV3);
        return AddV3(MulV3F(A, 1.0f - Time), MulV3F(B, Time));
    }

    COVERAGE(LerpV4, 1)
    static inline Vec4 LerpV4(Vec4 A, Vec4 B, Float Time)
    {
        ASSERT_COVERED(LerpV4);
        return AddV4(MulV4F(A, 1.0f - Time), MulV4F(B, Time));
    }

    COVERAGE(ClampV2, 1)
    static inline Vec2 ClampV2(Vec2 Value, Vec2 MinValue, Vec2 MaxValue)
    {
        ASSERT_COVERED(ClampV2);
        return V2(Clamp(Value.X, MinValue.X, MaxValue.X), Clamp(Value.Y, MinValue.Y, MaxValue.Y));
    }

    COVERAGE(ClampV3, 1)
    static inline Vec3 ClampV3(Vec3 Value, Vec3 MinValue, Vec3 MaxValue)
    {
        ASSERT_COVERED(ClampV3);
        return V3(Clamp(Value.X, MinValue.X, MaxValue.X), Clamp(Value.Y, MinValue.Y, MaxValue.Y), Clamp(Value.Z, MinValue.Z, MaxValue.Z));
    }

    COVERAGE(ClampV4, 1)
    static inline Vec4 ClampV4(Vec4 Value, Vec4 MinValue, Vec4 MaxValue)
    {
        ASSERT_COVERED(ClampV4);
        return V4(
            Clamp(Value.X, MinValue.X, MaxValue.X),
            Clamp(Value.Y, MinValue.Y, MaxValue.Y),
            Clamp(Value.Z, MinValue.Z, MaxValue.Z),
            Clamp(Value.W, MinValue.W, MaxValue.W));
    }

    COVERAGE(IsFiniteV2, 1)
    static inline Bool IsFiniteV2(Vec2 Value)
    {
        ASSERT_COVERED(IsFiniteV2)
        return IsFinite(Value.X) && IsFinite(Value.Y);
    }

    COVERAGE(IsFiniteV3, 1)
    static inline Bool IsFiniteV3(Vec3 Value)
    {
        ASSERT_COVERED(IsFiniteV3)
        return IsFinite(Value.X) && IsFinite(Value.Y) && IsFinite(Value.Z);
    }

    COVERAGE(IsFiniteV4, 1)
    static inline Bool IsFiniteV4(Vec4 Value)
    {
        ASSERT_COVERED(IsFiniteV4)
        return IsFinite(Value.X) && IsFinite(Value.Y) && IsFinite(Value.Z) && IsFinite(Value.W);
    }

    /*
     * SSE stuff
     */

    COVERAGE(LinearCombineV4M4, 1)
    static inline Vec4 LinearCombineV4M4(Vec4 Left, Mat4 Right)
    {
        ASSERT_COVERED(LinearCombineV4M4);

        Vec4 Result;
#ifdef HANDMADE_MATH__USE_SSE
        Result.SSE = _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0x00), Right.Columns[0].SSE);
        Result.SSE = _mm_add_ps(Result.SSE, _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0x55), Right.Columns[1].SSE));
        Result.SSE = _mm_add_ps(Result.SSE, _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0xaa), Right.Columns[2].SSE));
        Result.SSE = _mm_add_ps(Result.SSE, _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0xff), Right.Columns[3].SSE));
#else
    Result.X = Left.Data[0] * Right.Columns[0].X;
    Result.Y = Left.Data[0] * Right.Columns[0].Y;
    Result.Z = Left.Data[0] * Right.Columns[0].Z;
    Result.W = Left.Data[0] * Right.Columns[0].W;

    Result.X += Left.Data[1] * Right.Columns[1].X;
    Result.Y += Left.Data[1] * Right.Columns[1].Y;
    Result.Z += Left.Data[1] * Right.Columns[1].Z;
    Result.W += Left.Data[1] * Right.Columns[1].W;

    Result.X += Left.Data[2] * Right.Columns[2].X;
    Result.Y += Left.Data[2] * Right.Columns[2].Y;
    Result.Z += Left.Data[2] * Right.Columns[2].Z;
    Result.W += Left.Data[2] * Right.Columns[2].W;

    Result.X += Left.Data[3] * Right.Columns[3].X;
    Result.Y += Left.Data[3] * Right.Columns[3].Y;
    Result.Z += Left.Data[3] * Right.Columns[3].Z;
    Result.W += Left.Data[3] * Right.Columns[3].W;
#endif

        return Result;
    }

    /*
     * 2x2 Matrices
     */

    COVERAGE(M2, 1)
    static inline Mat2 M2(void)
    {
        ASSERT_COVERED(M2);
        Mat2 Result = {0};
        return Result;
    }

    COVERAGE(M2D, 1)
    static inline Mat2 M2D(Float Diagonal)
    {
        ASSERT_COVERED(M2D);

        Mat2 Result = {0};
        Result.Data[0][0] = Diagonal;
        Result.Data[1][1] = Diagonal;

        return Result;
    }

    COVERAGE(TransposeM2, 1)
    static inline Mat2 TransposeM2(Mat2 Matrix)
    {
        ASSERT_COVERED(TransposeM2);

        Mat2 Result = Matrix;

        Result.Data[0][1] = Matrix.Data[1][0];
        Result.Data[1][0] = Matrix.Data[0][1];

        return Result;
    }

    COVERAGE(AddM2, 1)
    static inline Mat2 AddM2(Mat2 Left, Mat2 Right)
    {
        ASSERT_COVERED(AddM2);

        Mat2 Result;

        Result.Data[0][0] = Left.Data[0][0] + Right.Data[0][0];
        Result.Data[0][1] = Left.Data[0][1] + Right.Data[0][1];
        Result.Data[1][0] = Left.Data[1][0] + Right.Data[1][0];
        Result.Data[1][1] = Left.Data[1][1] + Right.Data[1][1];

        return Result;
    }

    COVERAGE(SubM2, 1)
    static inline Mat2 SubM2(Mat2 Left, Mat2 Right)
    {
        ASSERT_COVERED(SubM2);

        Mat2 Result;

        Result.Data[0][0] = Left.Data[0][0] - Right.Data[0][0];
        Result.Data[0][1] = Left.Data[0][1] - Right.Data[0][1];
        Result.Data[1][0] = Left.Data[1][0] - Right.Data[1][0];
        Result.Data[1][1] = Left.Data[1][1] - Right.Data[1][1];

        return Result;
    }

    COVERAGE(MulM2V2, 1)
    static inline Vec2 MulM2V2(Mat2 Matrix, Vec2 Vector)
    {
        ASSERT_COVERED(MulM2V2);

        Vec2 Result;

        Result.X = Vector.Data[0] * Matrix.Columns[0].X;
        Result.Y = Vector.Data[0] * Matrix.Columns[0].Y;

        Result.X += Vector.Data[1] * Matrix.Columns[1].X;
        Result.Y += Vector.Data[1] * Matrix.Columns[1].Y;

        return Result;
    }

    COVERAGE(MulM2, 1)
    static inline Mat2 MulM2(Mat2 Left, Mat2 Right)
    {
        ASSERT_COVERED(MulM2);

        Mat2 Result;
        Result.Columns[0] = MulM2V2(Left, Right.Columns[0]);
        Result.Columns[1] = MulM2V2(Left, Right.Columns[1]);

        return Result;
    }

    COVERAGE(MulM2F, 1)
    static inline Mat2 MulM2F(Mat2 Matrix, Float Scalar)
    {
        ASSERT_COVERED(MulM2F);

        Mat2 Result;

        Result.Data[0][0] = Matrix.Data[0][0] * Scalar;
        Result.Data[0][1] = Matrix.Data[0][1] * Scalar;
        Result.Data[1][0] = Matrix.Data[1][0] * Scalar;
        Result.Data[1][1] = Matrix.Data[1][1] * Scalar;

        return Result;
    }

    COVERAGE(DivM2F, 1)
    static inline Mat2 DivM2F(Mat2 Matrix, Float Scalar)
    {
        ASSERT_COVERED(DivM2F);

        Mat2 Result;

        Result.Data[0][0] = Matrix.Data[0][0] / Scalar;
        Result.Data[0][1] = Matrix.Data[0][1] / Scalar;
        Result.Data[1][0] = Matrix.Data[1][0] / Scalar;
        Result.Data[1][1] = Matrix.Data[1][1] / Scalar;

        return Result;
    }

    COVERAGE(DeterminantM2, 1)
    static inline Float DeterminantM2(Mat2 Matrix)
    {
        ASSERT_COVERED(DeterminantM2);
        return Matrix.Data[0][0] * Matrix.Data[1][1] - Matrix.Data[0][1] * Matrix.Data[1][0];
    }

    COVERAGE(InvGeneralM2, 1)
    static inline Mat2 InvGeneralM2(Mat2 Matrix)
    {
        ASSERT_COVERED(InvGeneralM2);

        Mat2 Result;
        Float InvDeterminant = 1.0f / DeterminantM2(Matrix);
        Result.Data[0][0] = InvDeterminant * +Matrix.Data[1][1];
        Result.Data[1][1] = InvDeterminant * +Matrix.Data[0][0];
        Result.Data[0][1] = InvDeterminant * -Matrix.Data[0][1];
        Result.Data[1][0] = InvDeterminant * -Matrix.Data[1][0];

        return Result;
    }

    /*
     * 3x3 Matrices
     */

    COVERAGE(M3, 1)
    static inline Mat3 M3(void)
    {
        ASSERT_COVERED(M3);
        Mat3 Result = {0};
        return Result;
    }

    COVERAGE(M3D, 1)
    static inline Mat3 M3D(Float Diagonal)
    {
        ASSERT_COVERED(M3D);

        Mat3 Result = {0};
        Result.Data[0][0] = Diagonal;
        Result.Data[1][1] = Diagonal;
        Result.Data[2][2] = Diagonal;

        return Result;
    }

    COVERAGE(TransposeM3, 1)
    static inline Mat3 TransposeM3(Mat3 Matrix)
    {
        ASSERT_COVERED(TransposeM3);

        Mat3 Result = Matrix;

        Result.Data[0][1] = Matrix.Data[1][0];
        Result.Data[0][2] = Matrix.Data[2][0];
        Result.Data[1][0] = Matrix.Data[0][1];
        Result.Data[1][2] = Matrix.Data[2][1];
        Result.Data[2][1] = Matrix.Data[1][2];
        Result.Data[2][0] = Matrix.Data[0][2];

        return Result;
    }

    COVERAGE(AddM3, 1)
    static inline Mat3 AddM3(Mat3 Left, Mat3 Right)
    {
        ASSERT_COVERED(AddM3);

        Mat3 Result;

        Result.Data[0][0] = Left.Data[0][0] + Right.Data[0][0];
        Result.Data[0][1] = Left.Data[0][1] + Right.Data[0][1];
        Result.Data[0][2] = Left.Data[0][2] + Right.Data[0][2];
        Result.Data[1][0] = Left.Data[1][0] + Right.Data[1][0];
        Result.Data[1][1] = Left.Data[1][1] + Right.Data[1][1];
        Result.Data[1][2] = Left.Data[1][2] + Right.Data[1][2];
        Result.Data[2][0] = Left.Data[2][0] + Right.Data[2][0];
        Result.Data[2][1] = Left.Data[2][1] + Right.Data[2][1];
        Result.Data[2][2] = Left.Data[2][2] + Right.Data[2][2];

        return Result;
    }

    COVERAGE(SubM3, 1)
    static inline Mat3 SubM3(Mat3 Left, Mat3 Right)
    {
        ASSERT_COVERED(SubM3);

        Mat3 Result;

        Result.Data[0][0] = Left.Data[0][0] - Right.Data[0][0];
        Result.Data[0][1] = Left.Data[0][1] - Right.Data[0][1];
        Result.Data[0][2] = Left.Data[0][2] - Right.Data[0][2];
        Result.Data[1][0] = Left.Data[1][0] - Right.Data[1][0];
        Result.Data[1][1] = Left.Data[1][1] - Right.Data[1][1];
        Result.Data[1][2] = Left.Data[1][2] - Right.Data[1][2];
        Result.Data[2][0] = Left.Data[2][0] - Right.Data[2][0];
        Result.Data[2][1] = Left.Data[2][1] - Right.Data[2][1];
        Result.Data[2][2] = Left.Data[2][2] - Right.Data[2][2];

        return Result;
    }

    COVERAGE(MulM3V3, 1)
    static inline Vec3 MulM3V3(Mat3 Matrix, Vec3 Vector)
    {
        ASSERT_COVERED(MulM3V3);

        Vec3 Result;

        Result.X = Vector.Data[0] * Matrix.Columns[0].X;
        Result.Y = Vector.Data[0] * Matrix.Columns[0].Y;
        Result.Z = Vector.Data[0] * Matrix.Columns[0].Z;

        Result.X += Vector.Data[1] * Matrix.Columns[1].X;
        Result.Y += Vector.Data[1] * Matrix.Columns[1].Y;
        Result.Z += Vector.Data[1] * Matrix.Columns[1].Z;

        Result.X += Vector.Data[2] * Matrix.Columns[2].X;
        Result.Y += Vector.Data[2] * Matrix.Columns[2].Y;
        Result.Z += Vector.Data[2] * Matrix.Columns[2].Z;

        return Result;
    }

    COVERAGE(MulM3, 1)
    static inline Mat3 MulM3(Mat3 Left, Mat3 Right)
    {
        ASSERT_COVERED(MulM3);

        Mat3 Result;
        Result.Columns[0] = MulM3V3(Left, Right.Columns[0]);
        Result.Columns[1] = MulM3V3(Left, Right.Columns[1]);
        Result.Columns[2] = MulM3V3(Left, Right.Columns[2]);

        return Result;
    }

    COVERAGE(MulM3F, 1)
    static inline Mat3 MulM3F(Mat3 Matrix, Float Scalar)
    {
        ASSERT_COVERED(MulM3F);

        Mat3 Result;

        Result.Data[0][0] = Matrix.Data[0][0] * Scalar;
        Result.Data[0][1] = Matrix.Data[0][1] * Scalar;
        Result.Data[0][2] = Matrix.Data[0][2] * Scalar;
        Result.Data[1][0] = Matrix.Data[1][0] * Scalar;
        Result.Data[1][1] = Matrix.Data[1][1] * Scalar;
        Result.Data[1][2] = Matrix.Data[1][2] * Scalar;
        Result.Data[2][0] = Matrix.Data[2][0] * Scalar;
        Result.Data[2][1] = Matrix.Data[2][1] * Scalar;
        Result.Data[2][2] = Matrix.Data[2][2] * Scalar;

        return Result;
    }

    COVERAGE(DivM3, 1)
    static inline Mat3 DivM3F(Mat3 Matrix, Float Scalar)
    {
        ASSERT_COVERED(DivM3);

        Mat3 Result;

        Result.Data[0][0] = Matrix.Data[0][0] / Scalar;
        Result.Data[0][1] = Matrix.Data[0][1] / Scalar;
        Result.Data[0][2] = Matrix.Data[0][2] / Scalar;
        Result.Data[1][0] = Matrix.Data[1][0] / Scalar;
        Result.Data[1][1] = Matrix.Data[1][1] / Scalar;
        Result.Data[1][2] = Matrix.Data[1][2] / Scalar;
        Result.Data[2][0] = Matrix.Data[2][0] / Scalar;
        Result.Data[2][1] = Matrix.Data[2][1] / Scalar;
        Result.Data[2][2] = Matrix.Data[2][2] / Scalar;

        return Result;
    }

    COVERAGE(DeterminantM3, 1)
    static inline Float DeterminantM3(Mat3 Matrix)
    {
        ASSERT_COVERED(DeterminantM3);

        Mat3 Product;
        Product.Columns[0] = Cross(Matrix.Columns[1], Matrix.Columns[2]);
        Product.Columns[1] = Cross(Matrix.Columns[2], Matrix.Columns[0]);
        Product.Columns[2] = Cross(Matrix.Columns[0], Matrix.Columns[1]);

        return DotV3(Product.Columns[2], Matrix.Columns[2]);
    }

    COVERAGE(InvGeneralM3, 1)
    static inline Mat3 InvGeneralM3(Mat3 Matrix)
    {
        ASSERT_COVERED(InvGeneralM3);

        Mat3 Product;
        Product.Columns[0] = Cross(Matrix.Columns[1], Matrix.Columns[2]);
        Product.Columns[1] = Cross(Matrix.Columns[2], Matrix.Columns[0]);
        Product.Columns[2] = Cross(Matrix.Columns[0], Matrix.Columns[1]);

        Float InvDeterminant = 1.0f / DotV3(Product.Columns[2], Matrix.Columns[2]);

        Mat3 Result;
        Result.Columns[0] = MulV3F(Product.Columns[0], InvDeterminant);
        Result.Columns[1] = MulV3F(Product.Columns[1], InvDeterminant);
        Result.Columns[2] = MulV3F(Product.Columns[2], InvDeterminant);

        return TransposeM3(Result);
    }

    /*
     * 4x4 Matrices
     */

    COVERAGE(M4, 1)
    static inline Mat4 M4(void)
    {
        ASSERT_COVERED(M4);
        Mat4 Result = {0};
        return Result;
    }

    COVERAGE(M4D, 1)
    static inline Mat4 M4D(Float Diagonal)
    {
        ASSERT_COVERED(M4D);

        Mat4 Result = {0};
        Result.Data[0][0] = Diagonal;
        Result.Data[1][1] = Diagonal;
        Result.Data[2][2] = Diagonal;
        Result.Data[3][3] = Diagonal;

        return Result;
    }

    COVERAGE(TransposeM4, 1)
    static inline Mat4 TransposeM4(Mat4 Matrix)
    {
        ASSERT_COVERED(TransposeM4);

        Mat4 Result = Matrix;
#ifdef HANDMADE_MATH__USE_SSE
        _MM_TRANSPOSE4_PS(Result.Columns[0].SSE, Result.Columns[1].SSE, Result.Columns[2].SSE, Result.Columns[3].SSE);
#else
    Result.Data[0][1] = Matrix.Data[1][0];
    Result.Data[0][2] = Matrix.Data[2][0];
    Result.Data[0][3] = Matrix.Data[3][0];
    Result.Data[1][0] = Matrix.Data[0][1];
    Result.Data[1][2] = Matrix.Data[2][1];
    Result.Data[1][3] = Matrix.Data[3][1];
    Result.Data[2][1] = Matrix.Data[1][2];
    Result.Data[2][0] = Matrix.Data[0][2];
    Result.Data[2][3] = Matrix.Data[3][2];
    Result.Data[3][1] = Matrix.Data[1][3];
    Result.Data[3][2] = Matrix.Data[2][3];
    Result.Data[3][0] = Matrix.Data[0][3];
#endif

        return Result;
    }

    COVERAGE(AddM4, 1)
    static inline Mat4 AddM4(Mat4 Left, Mat4 Right)
    {
        ASSERT_COVERED(AddM4);

        Mat4 Result;

#ifdef HANDMADE_MATH__USE_SSE
        Result.Columns[0].SSE = _mm_add_ps(Left.Columns[0].SSE, Right.Columns[0].SSE);
        Result.Columns[1].SSE = _mm_add_ps(Left.Columns[1].SSE, Right.Columns[1].SSE);
        Result.Columns[2].SSE = _mm_add_ps(Left.Columns[2].SSE, Right.Columns[2].SSE);
        Result.Columns[3].SSE = _mm_add_ps(Left.Columns[3].SSE, Right.Columns[3].SSE);
#else
    Result.Data[0][0] = Left.Data[0][0] + Right.Data[0][0];
    Result.Data[0][1] = Left.Data[0][1] + Right.Data[0][1];
    Result.Data[0][2] = Left.Data[0][2] + Right.Data[0][2];
    Result.Data[0][3] = Left.Data[0][3] + Right.Data[0][3];
    Result.Data[1][0] = Left.Data[1][0] + Right.Data[1][0];
    Result.Data[1][1] = Left.Data[1][1] + Right.Data[1][1];
    Result.Data[1][2] = Left.Data[1][2] + Right.Data[1][2];
    Result.Data[1][3] = Left.Data[1][3] + Right.Data[1][3];
    Result.Data[2][0] = Left.Data[2][0] + Right.Data[2][0];
    Result.Data[2][1] = Left.Data[2][1] + Right.Data[2][1];
    Result.Data[2][2] = Left.Data[2][2] + Right.Data[2][2];
    Result.Data[2][3] = Left.Data[2][3] + Right.Data[2][3];
    Result.Data[3][0] = Left.Data[3][0] + Right.Data[3][0];
    Result.Data[3][1] = Left.Data[3][1] + Right.Data[3][1];
    Result.Data[3][2] = Left.Data[3][2] + Right.Data[3][2];
    Result.Data[3][3] = Left.Data[3][3] + Right.Data[3][3];
#endif

        return Result;
    }

    COVERAGE(SubM4, 1)
    static inline Mat4 SubM4(Mat4 Left, Mat4 Right)
    {
        ASSERT_COVERED(SubM4);

        Mat4 Result;

#ifdef HANDMADE_MATH__USE_SSE
        Result.Columns[0].SSE = _mm_sub_ps(Left.Columns[0].SSE, Right.Columns[0].SSE);
        Result.Columns[1].SSE = _mm_sub_ps(Left.Columns[1].SSE, Right.Columns[1].SSE);
        Result.Columns[2].SSE = _mm_sub_ps(Left.Columns[2].SSE, Right.Columns[2].SSE);
        Result.Columns[3].SSE = _mm_sub_ps(Left.Columns[3].SSE, Right.Columns[3].SSE);
#else
    Result.Data[0][0] = Left.Data[0][0] - Right.Data[0][0];
    Result.Data[0][1] = Left.Data[0][1] - Right.Data[0][1];
    Result.Data[0][2] = Left.Data[0][2] - Right.Data[0][2];
    Result.Data[0][3] = Left.Data[0][3] - Right.Data[0][3];
    Result.Data[1][0] = Left.Data[1][0] - Right.Data[1][0];
    Result.Data[1][1] = Left.Data[1][1] - Right.Data[1][1];
    Result.Data[1][2] = Left.Data[1][2] - Right.Data[1][2];
    Result.Data[1][3] = Left.Data[1][3] - Right.Data[1][3];
    Result.Data[2][0] = Left.Data[2][0] - Right.Data[2][0];
    Result.Data[2][1] = Left.Data[2][1] - Right.Data[2][1];
    Result.Data[2][2] = Left.Data[2][2] - Right.Data[2][2];
    Result.Data[2][3] = Left.Data[2][3] - Right.Data[2][3];
    Result.Data[3][0] = Left.Data[3][0] - Right.Data[3][0];
    Result.Data[3][1] = Left.Data[3][1] - Right.Data[3][1];
    Result.Data[3][2] = Left.Data[3][2] - Right.Data[3][2];
    Result.Data[3][3] = Left.Data[3][3] - Right.Data[3][3];
#endif

        return Result;
    }

    COVERAGE(MulM4, 1)
    static inline Mat4 MulM4(Mat4 Left, Mat4 Right)
    {
        ASSERT_COVERED(MulM4);

        Mat4 Result;
        Result.Columns[0] = LinearCombineV4M4(Right.Columns[0], Left);
        Result.Columns[1] = LinearCombineV4M4(Right.Columns[1], Left);
        Result.Columns[2] = LinearCombineV4M4(Right.Columns[2], Left);
        Result.Columns[3] = LinearCombineV4M4(Right.Columns[3], Left);

        return Result;
    }

    COVERAGE(MulM4F, 1)
    static inline Mat4 MulM4F(Mat4 Matrix, Float Scalar)
    {
        ASSERT_COVERED(MulM4F);

        Mat4 Result;

#ifdef HANDMADE_MATH__USE_SSE
        __m128 SSEScalar = _mm_set1_ps(Scalar);
        Result.Columns[0].SSE = _mm_mul_ps(Matrix.Columns[0].SSE, SSEScalar);
        Result.Columns[1].SSE = _mm_mul_ps(Matrix.Columns[1].SSE, SSEScalar);
        Result.Columns[2].SSE = _mm_mul_ps(Matrix.Columns[2].SSE, SSEScalar);
        Result.Columns[3].SSE = _mm_mul_ps(Matrix.Columns[3].SSE, SSEScalar);
#else
    Result.Data[0][0] = Matrix.Data[0][0] * Scalar;
    Result.Data[0][1] = Matrix.Data[0][1] * Scalar;
    Result.Data[0][2] = Matrix.Data[0][2] * Scalar;
    Result.Data[0][3] = Matrix.Data[0][3] * Scalar;
    Result.Data[1][0] = Matrix.Data[1][0] * Scalar;
    Result.Data[1][1] = Matrix.Data[1][1] * Scalar;
    Result.Data[1][2] = Matrix.Data[1][2] * Scalar;
    Result.Data[1][3] = Matrix.Data[1][3] * Scalar;
    Result.Data[2][0] = Matrix.Data[2][0] * Scalar;
    Result.Data[2][1] = Matrix.Data[2][1] * Scalar;
    Result.Data[2][2] = Matrix.Data[2][2] * Scalar;
    Result.Data[2][3] = Matrix.Data[2][3] * Scalar;
    Result.Data[3][0] = Matrix.Data[3][0] * Scalar;
    Result.Data[3][1] = Matrix.Data[3][1] * Scalar;
    Result.Data[3][2] = Matrix.Data[3][2] * Scalar;
    Result.Data[3][3] = Matrix.Data[3][3] * Scalar;
#endif

        return Result;
    }

    COVERAGE(MulM4V4, 1)
    static inline Vec4 MulM4V4(Mat4 Matrix, Vec4 Vector)
    {
        ASSERT_COVERED(MulM4V4);
        return LinearCombineV4M4(Vector, Matrix);
    }

    COVERAGE(DivM4F, 1)
    static inline Mat4 DivM4F(Mat4 Matrix, Float Scalar)
    {
        ASSERT_COVERED(DivM4F);

        Mat4 Result;

#ifdef HANDMADE_MATH__USE_SSE
        __m128 SSEScalar = _mm_set1_ps(Scalar);
        Result.Columns[0].SSE = _mm_div_ps(Matrix.Columns[0].SSE, SSEScalar);
        Result.Columns[1].SSE = _mm_div_ps(Matrix.Columns[1].SSE, SSEScalar);
        Result.Columns[2].SSE = _mm_div_ps(Matrix.Columns[2].SSE, SSEScalar);
        Result.Columns[3].SSE = _mm_div_ps(Matrix.Columns[3].SSE, SSEScalar);
#else
    Result.Data[0][0] = Matrix.Data[0][0] / Scalar;
    Result.Data[0][1] = Matrix.Data[0][1] / Scalar;
    Result.Data[0][2] = Matrix.Data[0][2] / Scalar;
    Result.Data[0][3] = Matrix.Data[0][3] / Scalar;
    Result.Data[1][0] = Matrix.Data[1][0] / Scalar;
    Result.Data[1][1] = Matrix.Data[1][1] / Scalar;
    Result.Data[1][2] = Matrix.Data[1][2] / Scalar;
    Result.Data[1][3] = Matrix.Data[1][3] / Scalar;
    Result.Data[2][0] = Matrix.Data[2][0] / Scalar;
    Result.Data[2][1] = Matrix.Data[2][1] / Scalar;
    Result.Data[2][2] = Matrix.Data[2][2] / Scalar;
    Result.Data[2][3] = Matrix.Data[2][3] / Scalar;
    Result.Data[3][0] = Matrix.Data[3][0] / Scalar;
    Result.Data[3][1] = Matrix.Data[3][1] / Scalar;
    Result.Data[3][2] = Matrix.Data[3][2] / Scalar;
    Result.Data[3][3] = Matrix.Data[3][3] / Scalar;
#endif

        return Result;
    }

    COVERAGE(DeterminantM4, 1)
    static inline Float DeterminantM4(Mat4 Matrix)
    {
        ASSERT_COVERED(DeterminantM4);

        Vec3 C01 = Cross(Matrix.Columns[0].XYZ, Matrix.Columns[1].XYZ);
        Vec3 C23 = Cross(Matrix.Columns[2].XYZ, Matrix.Columns[3].XYZ);
        Vec3 B10 = SubV3(MulV3F(Matrix.Columns[0].XYZ, Matrix.Columns[1].W), MulV3F(Matrix.Columns[1].XYZ, Matrix.Columns[0].W));
        Vec3 B32 = SubV3(MulV3F(Matrix.Columns[2].XYZ, Matrix.Columns[3].W), MulV3F(Matrix.Columns[3].XYZ, Matrix.Columns[2].W));

        return DotV3(C01, B32) + DotV3(C23, B10);
    }

    COVERAGE(InvGeneralM4, 1)
    // Returns a general-purpose inverse of an Mat4. Note that special-purpose inverses of many transformations
    // are available and will be more efficient.
    static inline Mat4 InvGeneralM4(Mat4 Matrix)
    {
        ASSERT_COVERED(InvGeneralM4);

        Vec3 C01 = Cross(Matrix.Columns[0].XYZ, Matrix.Columns[1].XYZ);
        Vec3 C23 = Cross(Matrix.Columns[2].XYZ, Matrix.Columns[3].XYZ);
        Vec3 B10 = SubV3(MulV3F(Matrix.Columns[0].XYZ, Matrix.Columns[1].W), MulV3F(Matrix.Columns[1].XYZ, Matrix.Columns[0].W));
        Vec3 B32 = SubV3(MulV3F(Matrix.Columns[2].XYZ, Matrix.Columns[3].W), MulV3F(Matrix.Columns[3].XYZ, Matrix.Columns[2].W));

        Float InvDeterminant = 1.0f / (DotV3(C01, B32) + DotV3(C23, B10));
        C01 = MulV3F(C01, InvDeterminant);
        C23 = MulV3F(C23, InvDeterminant);
        B10 = MulV3F(B10, InvDeterminant);
        B32 = MulV3F(B32, InvDeterminant);

        Mat4 Result;
        Result.Columns[0] = V4V(AddV3(Cross(Matrix.Columns[1].XYZ, B32), MulV3F(C23, Matrix.Columns[1].W)), -DotV3(Matrix.Columns[1].XYZ, C23));
        Result.Columns[1] = V4V(SubV3(Cross(B32, Matrix.Columns[0].XYZ), MulV3F(C23, Matrix.Columns[0].W)), +DotV3(Matrix.Columns[0].XYZ, C23));
        Result.Columns[2] = V4V(AddV3(Cross(Matrix.Columns[3].XYZ, B10), MulV3F(C01, Matrix.Columns[3].W)), -DotV3(Matrix.Columns[3].XYZ, C01));
        Result.Columns[3] = V4V(SubV3(Cross(B10, Matrix.Columns[2].XYZ), MulV3F(C01, Matrix.Columns[2].W)), +DotV3(Matrix.Columns[2].XYZ, C01));

        return TransposeM4(Result);
    }

    /*
     * Common graphics transformations
     */

    COVERAGE(Orthographic_RH_NO, 1)
    // Produces a right-handed orthographic projection matrix with Z ranging from -1 to 1 (the GL convention).
    // Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
    // Near and Far specify the distances to the near and far clipping planes.
    static inline Mat4 Orthographic_RH_NO(Float Left, Float Right, Float Bottom, Float Top, Float Near, Float Far)
    {
        ASSERT_COVERED(Orthographic_RH_NO);

        Mat4 Result = {0};

        Result.Data[0][0] = 2.0f / (Right - Left);
        Result.Data[1][1] = 2.0f / (Top - Bottom);
        Result.Data[2][2] = 2.0f / (Near - Far);
        Result.Data[3][3] = 1.0f;

        Result.Data[3][0] = (Left + Right) / (Left - Right);
        Result.Data[3][1] = (Bottom + Top) / (Bottom - Top);
        Result.Data[3][2] = (Near + Far) / (Near - Far);

        return Result;
    }

    COVERAGE(Orthographic_RH_ZO, 1)
    // Produces a right-handed orthographic projection matrix with Z ranging from 0 to 1 (the DirectX convention).
    // Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
    // Near and Far specify the distances to the near and far clipping planes.
    static inline Mat4 Orthographic_RH_ZO(Float Left, Float Right, Float Bottom, Float Top, Float Near, Float Far)
    {
        ASSERT_COVERED(Orthographic_RH_ZO);

        Mat4 Result = {0};

        Result.Data[0][0] = 2.0f / (Right - Left);
        Result.Data[1][1] = 2.0f / (Top - Bottom);
        Result.Data[2][2] = 1.0f / (Near - Far);
        Result.Data[3][3] = 1.0f;

        Result.Data[3][0] = (Left + Right) / (Left - Right);
        Result.Data[3][1] = (Bottom + Top) / (Bottom - Top);
        Result.Data[3][2] = (Near) / (Near - Far);

        return Result;
    }

    COVERAGE(Orthographic_LH_NO, 1)
    // Produces a left-handed orthographic projection matrix with Z ranging from -1 to 1 (the GL convention).
    // Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
    // Near and Far specify the distances to the near and far clipping planes.
    static inline Mat4 Orthographic_LH_NO(Float Left, Float Right, Float Bottom, Float Top, Float Near, Float Far)
    {
        ASSERT_COVERED(Orthographic_LH_NO);

        Mat4 Result = Orthographic_RH_NO(Left, Right, Bottom, Top, Near, Far);
        Result.Data[2][2] = -Result.Data[2][2];

        return Result;
    }

    COVERAGE(Orthographic_LH_ZO, 1)
    // Produces a left-handed orthographic projection matrix with Z ranging from 0 to 1 (the DirectX convention).
    // Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
    // Near and Far specify the distances to the near and far clipping planes.
    static inline Mat4 Orthographic_LH_ZO(Float Left, Float Right, Float Bottom, Float Top, Float Near, Float Far)
    {
        ASSERT_COVERED(Orthographic_LH_ZO);

        Mat4 Result = Orthographic_RH_ZO(Left, Right, Bottom, Top, Near, Far);
        Result.Data[2][2] = -Result.Data[2][2];

        return Result;
    }

    COVERAGE(InvOrthographic, 1)
    // Returns an inverse for the given orthographic projection matrix. Works for all orthographic
    // projection matrices, regardless of handedness or NDC convention.
    static inline Mat4 InvOrthographic(Mat4 OrthoMatrix)
    {
        ASSERT_COVERED(InvOrthographic);

        Mat4 Result = {0};
        Result.Data[0][0] = 1.0f / OrthoMatrix.Data[0][0];
        Result.Data[1][1] = 1.0f / OrthoMatrix.Data[1][1];
        Result.Data[2][2] = 1.0f / OrthoMatrix.Data[2][2];
        Result.Data[3][3] = 1.0f;

        Result.Data[3][0] = -OrthoMatrix.Data[3][0] * Result.Data[0][0];
        Result.Data[3][1] = -OrthoMatrix.Data[3][1] * Result.Data[1][1];
        Result.Data[3][2] = -OrthoMatrix.Data[3][2] * Result.Data[2][2];

        return Result;
    }

    COVERAGE(Perspective_RH_NO, 1)
    static inline Mat4 Perspective_RH_NO(Float FOV, Float AspectRatio, Float Near, Float Far)
    {
        ASSERT_COVERED(Perspective_RH_NO);

        Mat4 Result = {0};

        // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

        Float Cotangent = 1.0f / TanF(FOV / 2.0f);
        Result.Data[0][0] = Cotangent / AspectRatio;
        Result.Data[1][1] = Cotangent;
        Result.Data[2][3] = -1.0f;

        Result.Data[2][2] = (Near + Far) / (Near - Far);
        Result.Data[3][2] = (2.0f * Near * Far) / (Near - Far);

        return Result;
    }

    COVERAGE(Perspective_RH_ZO, 1)
    static inline Mat4 Perspective_RH_ZO(Float FOV, Float AspectRatio, Float Near, Float Far)
    {
        ASSERT_COVERED(Perspective_RH_ZO);

        Mat4 Result = {0};

        // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

        Float Cotangent = 1.0f / TanF(FOV / 2.0f);
        Result.Data[0][0] = Cotangent / AspectRatio;
        Result.Data[1][1] = Cotangent;
        Result.Data[2][3] = -1.0f;

        Result.Data[2][2] = (Far) / (Near - Far);
        Result.Data[3][2] = (Near * Far) / (Near - Far);

        return Result;
    }

    COVERAGE(Perspective_LH_NO, 1)
    static inline Mat4 Perspective_LH_NO(Float FOV, Float AspectRatio, Float Near, Float Far)
    {
        ASSERT_COVERED(Perspective_LH_NO);

        Mat4 Result = Perspective_RH_NO(FOV, AspectRatio, Near, Far);
        Result.Data[2][2] = -Result.Data[2][2];
        Result.Data[2][3] = -Result.Data[2][3];

        return Result;
    }

    COVERAGE(Perspective_LH_ZO, 1)
    static inline Mat4 Perspective_LH_ZO(Float FOV, Float AspectRatio, Float Near, Float Far)
    {
        ASSERT_COVERED(Perspective_LH_ZO);

        Mat4 Result = Perspective_RH_ZO(FOV, AspectRatio, Near, Far);
        Result.Data[2][2] = -Result.Data[2][2];
        Result.Data[2][3] = -Result.Data[2][3];

        return Result;
    }

    COVERAGE(InvPerspective_RH, 1)
    static inline Mat4 InvPerspective_RH(Mat4 PerspectiveMatrix)
    {
        ASSERT_COVERED(InvPerspective_RH);

        Mat4 Result = {0};
        Result.Data[0][0] = 1.0f / PerspectiveMatrix.Data[0][0];
        Result.Data[1][1] = 1.0f / PerspectiveMatrix.Data[1][1];
        Result.Data[2][2] = 0.0f;

        Result.Data[2][3] = 1.0f / PerspectiveMatrix.Data[3][2];
        Result.Data[3][3] = PerspectiveMatrix.Data[2][2] * Result.Data[2][3];
        Result.Data[3][2] = PerspectiveMatrix.Data[2][3];

        return Result;
    }

    COVERAGE(InvPerspective_LH, 1)
    static inline Mat4 InvPerspective_LH(Mat4 PerspectiveMatrix)
    {
        ASSERT_COVERED(InvPerspective_LH);

        Mat4 Result = {0};
        Result.Data[0][0] = 1.0f / PerspectiveMatrix.Data[0][0];
        Result.Data[1][1] = 1.0f / PerspectiveMatrix.Data[1][1];
        Result.Data[2][2] = 0.0f;

        Result.Data[2][3] = 1.0f / PerspectiveMatrix.Data[3][2];
        Result.Data[3][3] = PerspectiveMatrix.Data[2][2] * -Result.Data[2][3];
        Result.Data[3][2] = PerspectiveMatrix.Data[2][3];

        return Result;
    }

    COVERAGE(TranslateV3, 1)
    static inline Mat4 TranslateV3(Vec3 Translation)
    {
        ASSERT_COVERED(TranslateV3);

        Mat4 Result = M4D(1.0f);
        Result.Data[3][0] = Translation.X;
        Result.Data[3][1] = Translation.Y;
        Result.Data[3][2] = Translation.Z;

        return Result;
    }

    COVERAGE(InvTranslate, 1)
    static inline Mat4 InvTranslate(Mat4 TranslationMatrix)
    {
        ASSERT_COVERED(InvTranslate);

        Mat4 Result = TranslationMatrix;
        Result.Data[3][0] = -Result.Data[3][0];
        Result.Data[3][1] = -Result.Data[3][1];
        Result.Data[3][2] = -Result.Data[3][2];

        return Result;
    }

    COVERAGE(Rotate_RH, 1)
    static inline Mat4 Rotate_RH(Float Angle, Vec3 Axis)
    {
        ASSERT_COVERED(Rotate_RH);

        Mat4 Result = M4D(1.0f);

        Axis = NormV3(Axis);

        Float SinTheta = SinF(Angle);
        Float CosTheta = CosF(Angle);
        Float CosValue = 1.0f - CosTheta;

        Result.Data[0][0] = (Axis.X * Axis.X * CosValue) + CosTheta;
        Result.Data[0][1] = (Axis.X * Axis.Y * CosValue) + (Axis.Z * SinTheta);
        Result.Data[0][2] = (Axis.X * Axis.Z * CosValue) - (Axis.Y * SinTheta);

        Result.Data[1][0] = (Axis.Y * Axis.X * CosValue) - (Axis.Z * SinTheta);
        Result.Data[1][1] = (Axis.Y * Axis.Y * CosValue) + CosTheta;
        Result.Data[1][2] = (Axis.Y * Axis.Z * CosValue) + (Axis.X * SinTheta);

        Result.Data[2][0] = (Axis.Z * Axis.X * CosValue) + (Axis.Y * SinTheta);
        Result.Data[2][1] = (Axis.Z * Axis.Y * CosValue) - (Axis.X * SinTheta);
        Result.Data[2][2] = (Axis.Z * Axis.Z * CosValue) + CosTheta;

        return Result;
    }

    COVERAGE(Rotate_LH, 1)
    static inline Mat4 Rotate_LH(Float Angle, Vec3 Axis)
    {
        ASSERT_COVERED(Rotate_LH);
        /* NOTE(lcf): Matrix will be inverse/transpose of RH. */
        return Rotate_RH(-Angle, Axis);
    }

    COVERAGE(InvRotate, 1)
    static inline Mat4 InvRotate(Mat4 RotationMatrix)
    {
        ASSERT_COVERED(InvRotate);
        return TransposeM4(RotationMatrix);
    }

    COVERAGE(ScaleV3, 1)
    static inline Mat4 ScaleV3(Vec3 Scale)
    {
        ASSERT_COVERED(ScaleV3);

        Mat4 Result = M4D(1.0f);
        Result.Data[0][0] = Scale.X;
        Result.Data[1][1] = Scale.Y;
        Result.Data[2][2] = Scale.Z;

        return Result;
    }

    COVERAGE(InvScale, 1)
    static inline Mat4 InvScale(Mat4 ScaleMatrix)
    {
        ASSERT_COVERED(InvScale);

        Mat4 Result = ScaleMatrix;
        Result.Data[0][0] = 1.0f / Result.Data[0][0];
        Result.Data[1][1] = 1.0f / Result.Data[1][1];
        Result.Data[2][2] = 1.0f / Result.Data[2][2];

        return Result;
    }

    static inline Mat4 _LookAt(Vec3 F, Vec3 S, Vec3 U, Vec3 Eye)
    {
        Mat4 Result;

        Result.Data[0][0] = S.X;
        Result.Data[0][1] = U.X;
        Result.Data[0][2] = -F.X;
        Result.Data[0][3] = 0.0f;

        Result.Data[1][0] = S.Y;
        Result.Data[1][1] = U.Y;
        Result.Data[1][2] = -F.Y;
        Result.Data[1][3] = 0.0f;

        Result.Data[2][0] = S.Z;
        Result.Data[2][1] = U.Z;
        Result.Data[2][2] = -F.Z;
        Result.Data[2][3] = 0.0f;

        Result.Data[3][0] = -DotV3(S, Eye);
        Result.Data[3][1] = -DotV3(U, Eye);
        Result.Data[3][2] = DotV3(F, Eye);
        Result.Data[3][3] = 1.0f;

        return Result;
    }

    COVERAGE(LookAt_RH, 1)
    static inline Mat4 LookAt_RH(Vec3 Eye, Vec3 Center, Vec3 Up)
    {
        ASSERT_COVERED(LookAt_RH);

        Vec3 F = NormV3(SubV3(Center, Eye));
        Vec3 S = NormV3(Cross(F, Up));
        Vec3 U = Cross(S, F);

        return _LookAt(F, S, U, Eye);
    }

    COVERAGE(LookAt_LH, 1)
    static inline Mat4 LookAt_LH(Vec3 Eye, Vec3 Center, Vec3 Up)
    {
        ASSERT_COVERED(LookAt_LH);

        Vec3 F = NormV3(SubV3(Eye, Center));
        Vec3 S = NormV3(Cross(F, Up));
        Vec3 U = Cross(S, F);

        return _LookAt(F, S, U, Eye);
    }

    COVERAGE(InvLookAt, 1)
    static inline Mat4 InvLookAt(Mat4 Matrix)
    {
        ASSERT_COVERED(InvLookAt);
        Mat4 Result;

        Mat3 Rotation = {0};
        Rotation.Columns[0] = Matrix.Columns[0].XYZ;
        Rotation.Columns[1] = Matrix.Columns[1].XYZ;
        Rotation.Columns[2] = Matrix.Columns[2].XYZ;
        Rotation = TransposeM3(Rotation);

        Result.Columns[0] = V4V(Rotation.Columns[0], 0.0f);
        Result.Columns[1] = V4V(Rotation.Columns[1], 0.0f);
        Result.Columns[2] = V4V(Rotation.Columns[2], 0.0f);
        Result.Columns[3] = MulV4F(Matrix.Columns[3], -1.0f);
        Result.Data[3][0] = -1.0f * Matrix.Data[3][0] /
                                (Rotation.Data[0][0] + Rotation.Data[0][1] + Rotation.Data[0][2]);
        Result.Data[3][1] = -1.0f * Matrix.Data[3][1] /
                                (Rotation.Data[1][0] + Rotation.Data[1][1] + Rotation.Data[1][2]);
        Result.Data[3][2] = -1.0f * Matrix.Data[3][2] /
                                (Rotation.Data[2][0] + Rotation.Data[2][1] + Rotation.Data[2][2]);
        Result.Data[3][3] = 1.0f;

        return Result;
    }

    /*
     * Quaternion operations
     */

    COVERAGE(Q, 1)
    static inline Quat Q(Float X, Float Y, Float Z, Float W)
    {
        ASSERT_COVERED(Q);

        Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
        Result.SSE = _mm_setr_ps(X, Y, Z, W);
#else
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
    Result.W = W;
#endif

        return Result;
    }

    COVERAGE(QV4, 1)
    static inline Quat QV4(Vec4 Vector)
    {
        ASSERT_COVERED(QV4);

        Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
        Result.SSE = Vector.SSE;
#else
    Result.X = Vector.X;
    Result.Y = Vector.Y;
    Result.Z = Vector.Z;
    Result.W = Vector.W;
#endif

        return Result;
    }

    COVERAGE(AddQ, 1)
    static inline Quat AddQ(Quat Left, Quat Right)
    {
        ASSERT_COVERED(AddQ);

        Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
        Result.SSE = _mm_add_ps(Left.SSE, Right.SSE);
#else

    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    Result.Z = Left.Z + Right.Z;
    Result.W = Left.W + Right.W;
#endif

        return Result;
    }

    COVERAGE(SubQ, 1)
    static inline Quat SubQ(Quat Left, Quat Right)
    {
        ASSERT_COVERED(SubQ);

        Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
        Result.SSE = _mm_sub_ps(Left.SSE, Right.SSE);
#else
    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    Result.Z = Left.Z - Right.Z;
    Result.W = Left.W - Right.W;
#endif

        return Result;
    }

    COVERAGE(MulQ, 1)
    static inline Quat MulQ(Quat Left, Quat Right)
    {
        ASSERT_COVERED(MulQ);

        Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
        __m128 SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(0, 0, 0, 0)), _mm_setr_ps(0.f, -0.f, 0.f, -0.f));
        __m128 SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(0, 1, 2, 3));
        __m128 SSEResultThree = _mm_mul_ps(SSEResultTwo, SSEResultOne);

        SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(1, 1, 1, 1)), _mm_setr_ps(0.f, 0.f, -0.f, -0.f));
        SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(1, 0, 3, 2));
        SSEResultThree = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));

        SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(2, 2, 2, 2)), _mm_setr_ps(-0.f, 0.f, 0.f, -0.f));
        SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(2, 3, 0, 1));
        SSEResultThree = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));

        SSEResultOne = _mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(3, 3, 3, 3));
        SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(3, 2, 1, 0));
        Result.SSE = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));
#else
    Result.X = Right.Data[3] * +Left.Data[0];
    Result.Y = Right.Data[2] * -Left.Data[0];
    Result.Z = Right.Data[1] * +Left.Data[0];
    Result.W = Right.Data[0] * -Left.Data[0];

    Result.X += Right.Data[2] * +Left.Data[1];
    Result.Y += Right.Data[3] * +Left.Data[1];
    Result.Z += Right.Data[0] * -Left.Data[1];
    Result.W += Right.Data[1] * -Left.Data[1];

    Result.X += Right.Data[1] * -Left.Data[2];
    Result.Y += Right.Data[0] * +Left.Data[2];
    Result.Z += Right.Data[3] * +Left.Data[2];
    Result.W += Right.Data[2] * -Left.Data[2];

    Result.X += Right.Data[0] * +Left.Data[3];
    Result.Y += Right.Data[1] * +Left.Data[3];
    Result.Z += Right.Data[2] * +Left.Data[3];
    Result.W += Right.Data[3] * +Left.Data[3];
#endif

        return Result;
    }

    COVERAGE(MulQF, 1)
    static inline Quat MulQF(Quat Left, Float Multiplicative)
    {
        ASSERT_COVERED(MulQF);

        Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
        __m128 Scalar = _mm_set1_ps(Multiplicative);
        Result.SSE = _mm_mul_ps(Left.SSE, Scalar);
#else
    Result.X = Left.X * Multiplicative;
    Result.Y = Left.Y * Multiplicative;
    Result.Z = Left.Z * Multiplicative;
    Result.W = Left.W * Multiplicative;
#endif

        return Result;
    }

    COVERAGE(DivQF, 1)
    static inline Quat DivQF(Quat Left, Float Divnd)
    {
        ASSERT_COVERED(DivQF);

        Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
        __m128 Scalar = _mm_set1_ps(Divnd);
        Result.SSE = _mm_div_ps(Left.SSE, Scalar);
#else
    Result.X = Left.X / Divnd;
    Result.Y = Left.Y / Divnd;
    Result.Z = Left.Z / Divnd;
    Result.W = Left.W / Divnd;
#endif

        return Result;
    }

    COVERAGE(DotQ, 1)
    static inline Float DotQ(Quat Left, Quat Right)
    {
        ASSERT_COVERED(DotQ);

        Float Result;

#ifdef HANDMADE_MATH__USE_SSE
        __m128 SSEResultOne = _mm_mul_ps(Left.SSE, Right.SSE);
        __m128 SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
        SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
        SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
        SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
        _mm_store_ss(&Result, SSEResultOne);
#else
    Result = ((Left.X * Right.X) + (Left.Z * Right.Z)) + ((Left.Y * Right.Y) + (Left.W * Right.W));
#endif

        return Result;
    }

    COVERAGE(InvQ, 1)
    static inline Quat InvQ(Quat Left)
    {
        ASSERT_COVERED(InvQ);

        Quat Result;
        Result.X = -Left.X;
        Result.Y = -Left.Y;
        Result.Z = -Left.Z;
        Result.W = Left.W;

        return DivQF(Result, (DotQ(Left, Left)));
    }

    COVERAGE(NormQ, 1)
    static inline Quat NormQ(Quat quat)
    {
        ASSERT_COVERED(NormQ);

        /* NOTE(lcf): Take advantage of SSE implementation in NormV4 */
        Vec4 Vec = {quat.X, quat.Y, quat.Z, quat.W};
        Vec = NormV4(Vec);
        Quat Result = {Vec.X, Vec.Y, Vec.Z, Vec.W};

        return Result;
    }

    static inline Quat _MixQ(Quat Left, Float MixLeft, Quat Right, Float MixRight)
    {
        Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
        __m128 ScalarLeft = _mm_set1_ps(MixLeft);
        __m128 ScalarRight = _mm_set1_ps(MixRight);
        __m128 SSEResultOne = _mm_mul_ps(Left.SSE, ScalarLeft);
        __m128 SSEResultTwo = _mm_mul_ps(Right.SSE, ScalarRight);
        Result.SSE = _mm_add_ps(SSEResultOne, SSEResultTwo);
#else
    Result.X = Left.X * MixLeft + Right.X * MixRight;
    Result.Y = Left.Y * MixLeft + Right.Y * MixRight;
    Result.Z = Left.Z * MixLeft + Right.Z * MixRight;
    Result.W = Left.W * MixLeft + Right.W * MixRight;
#endif

        return Result;
    }

    COVERAGE(NLerp, 1)
    static inline Quat NLerp(Quat Left, Quat Right, Float Time)
    {
        ASSERT_COVERED(NLerp);

        Quat Result = _MixQ(Left, 1.0f - Time, Right, Time);
        Result = NormQ(Result);

        return Result;
    }

    COVERAGE(SLerp, 1)
    static inline Quat SLerp(Quat Left, Quat Right, Float Time)
    {
        ASSERT_COVERED(SLerp);

        Quat Result;

        Float Cos_Theta = DotQ(Left, Right);

        if (Cos_Theta < 0.0f)
        { /* NOTE(lcf): Take shortest path on Hyper-sphere */
            Cos_Theta = -Cos_Theta;
            Right = Q(-Right.X, -Right.Y, -Right.Z, -Right.W);
        }

        /* NOTE(lcf): Use Normalized Linear interpolation when vectors are roughly not L.I. */
        if (Cos_Theta > 0.9995f)
        {
            Result = NLerp(Left, Right, Time);
        }
        else
        {
            Float Angle = ACosF(Cos_Theta);
            Float MixLeft = SinF((1.0f - Time) * Angle);
            Float MixRight = SinF(Time * Angle);

            Result = _MixQ(Left, MixLeft, Right, MixRight);
            Result = NormQ(Result);
        }

        return Result;
    }

    COVERAGE(M4FromQ, 1)
    static inline Mat4 M4FromQ(Quat Left)
    {
        ASSERT_COVERED(M4FromQ);

        Mat4 Result;

        Quat NormalizedQ = NormQ(Left);

        Float XX, YY, ZZ,
            XY, XZ, YZ,
            WX, WY, WZ;

        XX = NormalizedQ.X * NormalizedQ.X;
        YY = NormalizedQ.Y * NormalizedQ.Y;
        ZZ = NormalizedQ.Z * NormalizedQ.Z;
        XY = NormalizedQ.X * NormalizedQ.Y;
        XZ = NormalizedQ.X * NormalizedQ.Z;
        YZ = NormalizedQ.Y * NormalizedQ.Z;
        WX = NormalizedQ.W * NormalizedQ.X;
        WY = NormalizedQ.W * NormalizedQ.Y;
        WZ = NormalizedQ.W * NormalizedQ.Z;

        Result.Data[0][0] = 1.0f - 2.0f * (YY + ZZ);
        Result.Data[0][1] = 2.0f * (XY + WZ);
        Result.Data[0][2] = 2.0f * (XZ - WY);
        Result.Data[0][3] = 0.0f;

        Result.Data[1][0] = 2.0f * (XY - WZ);
        Result.Data[1][1] = 1.0f - 2.0f * (XX + ZZ);
        Result.Data[1][2] = 2.0f * (YZ + WX);
        Result.Data[1][3] = 0.0f;

        Result.Data[2][0] = 2.0f * (XZ + WY);
        Result.Data[2][1] = 2.0f * (YZ - WX);
        Result.Data[2][2] = 1.0f - 2.0f * (XX + YY);
        Result.Data[2][3] = 0.0f;

        Result.Data[3][0] = 0.0f;
        Result.Data[3][1] = 0.0f;
        Result.Data[3][2] = 0.0f;
        Result.Data[3][3] = 1.0f;

        return Result;
    }

    // This method taken from Mike Day at Insomniac Games.
    // https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2015/01/matrix-to-quat.pdf
    //
    // Note that as mentioned at the top of the paper, the paper assumes the matrix
    // would be *post*-multiplied to a vector to rotate it, meaning the matrix is
    // the transpose of what we're dealing with. But, because our matrices are
    // stored in column-major order, the indices *appear* to match the paper.
    //
    // For example, m12 in the paper is row 1, column 2. We need to transpose it to
    // row 2, column 1. But, because the column comes first when referencing
    // elements, it looks like M.Data[1][2].
    //
    // Don't be confused! Or if you must be confused, at least trust this
    // comment. :)
    COVERAGE(M4ToQ_RH, 4)
    static inline Quat M4ToQ_RH(Mat4 M)
    {
        Float T;
        Quat Result;

        if (M.Data[2][2] < 0.0f)
        {
            if (M.Data[0][0] > M.Data[1][1])
            {
                ASSERT_COVERED(M4ToQ_RH);

                T = 1 + M.Data[0][0] - M.Data[1][1] - M.Data[2][2];
                Result = Q(
                    T,
                    M.Data[0][1] + M.Data[1][0],
                    M.Data[2][0] + M.Data[0][2],
                    M.Data[1][2] - M.Data[2][1]);
            }
            else
            {
                ASSERT_COVERED(M4ToQ_RH);

                T = 1 - M.Data[0][0] + M.Data[1][1] - M.Data[2][2];
                Result = Q(
                    M.Data[0][1] + M.Data[1][0],
                    T,
                    M.Data[1][2] + M.Data[2][1],
                    M.Data[2][0] - M.Data[0][2]);
            }
        }
        else
        {
            if (M.Data[0][0] < -M.Data[1][1])
            {
                ASSERT_COVERED(M4ToQ_RH);

                T = 1 - M.Data[0][0] - M.Data[1][1] + M.Data[2][2];
                Result = Q(
                    M.Data[2][0] + M.Data[0][2],
                    M.Data[1][2] + M.Data[2][1],
                    T,
                    M.Data[0][1] - M.Data[1][0]);
            }
            else
            {
                ASSERT_COVERED(M4ToQ_RH);

                T = 1 + M.Data[0][0] + M.Data[1][1] + M.Data[2][2];
                Result = Q(
                    M.Data[1][2] - M.Data[2][1],
                    M.Data[2][0] - M.Data[0][2],
                    M.Data[0][1] - M.Data[1][0],
                    T);
            }
        }

        Result = MulQF(Result, 0.5f / SqrtF(T));

        return Result;
    }

    COVERAGE(M4ToQ_LH, 4)
    static inline Quat M4ToQ_LH(Mat4 M)
    {
        Float T;
        Quat Result;

        if (M.Data[2][2] < 0.0f)
        {
            if (M.Data[0][0] > M.Data[1][1])
            {
                ASSERT_COVERED(M4ToQ_LH);

                T = 1 + M.Data[0][0] - M.Data[1][1] - M.Data[2][2];
                Result = Q(
                    T,
                    M.Data[0][1] + M.Data[1][0],
                    M.Data[2][0] + M.Data[0][2],
                    M.Data[2][1] - M.Data[1][2]);
            }
            else
            {
                ASSERT_COVERED(M4ToQ_LH);

                T = 1 - M.Data[0][0] + M.Data[1][1] - M.Data[2][2];
                Result = Q(
                    M.Data[0][1] + M.Data[1][0],
                    T,
                    M.Data[1][2] + M.Data[2][1],
                    M.Data[0][2] - M.Data[2][0]);
            }
        }
        else
        {
            if (M.Data[0][0] < -M.Data[1][1])
            {
                ASSERT_COVERED(M4ToQ_LH);

                T = 1 - M.Data[0][0] - M.Data[1][1] + M.Data[2][2];
                Result = Q(
                    M.Data[2][0] + M.Data[0][2],
                    M.Data[1][2] + M.Data[2][1],
                    T,
                    M.Data[1][0] - M.Data[0][1]);
            }
            else
            {
                ASSERT_COVERED(M4ToQ_LH);

                T = 1 + M.Data[0][0] + M.Data[1][1] + M.Data[2][2];
                Result = Q(
                    M.Data[2][1] - M.Data[1][2],
                    M.Data[0][2] - M.Data[2][0],
                    M.Data[1][0] - M.Data[0][2],
                    T);
            }
        }

        Result = MulQF(Result, 0.5f / SqrtF(T));

        return Result;
    }

    COVERAGE(QFromAxisAngle_RH, 1)
    static inline Quat QFromAxisAngle_RH(Vec3 Axis, Float AngleOfRotation)
    {
        ASSERT_COVERED(QFromAxisAngle_RH);

        Quat Result;

        Vec3 AxisNormalized = NormV3(Axis);
        Float SineOfRotation = SinF(AngleOfRotation / 2.0f);

        Result.XYZ = MulV3F(AxisNormalized, SineOfRotation);
        Result.W = CosF(AngleOfRotation / 2.0f);

        return Result;
    }

    COVERAGE(QFromAxisAngle_LH, 1)
    static inline Quat QFromAxisAngle_LH(Vec3 Axis, Float AngleOfRotation)
    {
        ASSERT_COVERED(QFromAxisAngle_LH);

        return QFromAxisAngle_RH(Axis, -AngleOfRotation);
    }

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

COVERAGE(LenV2CPP, 1)
static inline Float Len(Vec2 A)
{
    ASSERT_COVERED(LenV2CPP);
    return LenV2(A);
}

COVERAGE(LenV3CPP, 1)
static inline Float Len(Vec3 A)
{
    ASSERT_COVERED(LenV3CPP);
    return LenV3(A);
}

COVERAGE(LenV4CPP, 1)
static inline Float Len(Vec4 A)
{
    ASSERT_COVERED(LenV4CPP);
    return LenV4(A);
}

COVERAGE(LenSqrV2CPP, 1)
static inline Float LenSqr(Vec2 A)
{
    ASSERT_COVERED(LenSqrV2CPP);
    return LenSqrV2(A);
}

COVERAGE(LenSqrV3CPP, 1)
static inline Float LenSqr(Vec3 A)
{
    ASSERT_COVERED(LenSqrV3CPP);
    return LenSqrV3(A);
}

COVERAGE(LenSqrV4CPP, 1)
static inline Float LenSqr(Vec4 A)
{
    ASSERT_COVERED(LenSqrV4CPP);
    return LenSqrV4(A);
}

COVERAGE(NormV2CPP, 1)
static inline Vec2 Norm(Vec2 A)
{
    ASSERT_COVERED(NormV2CPP);
    return NormV2(A);
}

COVERAGE(NormV3CPP, 1)
static inline Vec3 Norm(Vec3 A)
{
    ASSERT_COVERED(NormV3CPP);
    return NormV3(A);
}

COVERAGE(NormV4CPP, 1)
static inline Vec4 Norm(Vec4 A)
{
    ASSERT_COVERED(NormV4CPP);
    return NormV4(A);
}

COVERAGE(NormQCPP, 1)
static inline Quat Norm(Quat A)
{
    ASSERT_COVERED(NormQCPP);
    return NormQ(A);
}

COVERAGE(DotV2CPP, 1)
static inline Float Dot(Vec2 Left, Vec2 VecTwo)
{
    ASSERT_COVERED(DotV2CPP);
    return DotV2(Left, VecTwo);
}

COVERAGE(DotV3CPP, 1)
static inline Float Dot(Vec3 Left, Vec3 VecTwo)
{
    ASSERT_COVERED(DotV3CPP);
    return DotV3(Left, VecTwo);
}

COVERAGE(DotV4CPP, 1)
static inline Float Dot(Vec4 Left, Vec4 VecTwo)
{
    ASSERT_COVERED(DotV4CPP);
    return DotV4(Left, VecTwo);
}

COVERAGE(MinV2CPP, 1)
static inline Vec2 Min(Vec2 A, Vec2 B)
{
    ASSERT_COVERED(MinV2CPP);
    return MinV2(A, B);
}

COVERAGE(MinV3CPP, 1)
static inline Vec3 Min(Vec3 A, Vec3 B)
{
    ASSERT_COVERED(MinV3CPP);
    return MinV3(A, B);
}

COVERAGE(MinV4CPP, 1)
static inline Vec4 Min(Vec4 A, Vec4 B)
{
    ASSERT_COVERED(MinV4CPP);
    return MinV4(A, B);
}

COVERAGE(MaxV2CPP, 1)
static inline Vec2 Max(Vec2 A, Vec2 B)
{
    ASSERT_COVERED(MaxV2CPP);
    return MaxV2(A, B);
}

COVERAGE(MaxV3CPP, 1)
static inline Vec3 Max(Vec3 A, Vec3 B)
{
    ASSERT_COVERED(MaxV3CPP);
    return MaxV3(A, B);
}

COVERAGE(MaxV4CPP, 1)
static inline Vec4 Max(Vec4 A, Vec4 B)
{
    ASSERT_COVERED(MaxV4CPP);
    return MaxV4(A, B);
}

COVERAGE(AbsV2CPP, 1)
static inline Vec2 Abs(Vec2 V)
{
    ASSERT_COVERED(AbsV2CPP);
    return AbsV2(V);
}

COVERAGE(AbsV3CPP, 1)
static inline Vec3 Abs(Vec3 V)
{
    ASSERT_COVERED(AbsV3CPP);
    return AbsV3(V);
}

COVERAGE(AbsV4CPP, 1)
static inline Vec4 Abs(Vec4 V)
{
    ASSERT_COVERED(AbsV4CPP);
    return AbsV4(V);
}

COVERAGE(LerpV2CPP, 1)
static inline Vec2 Lerp(Vec2 Left, Vec2 Right, Float Time)
{
    ASSERT_COVERED(LerpV2CPP);
    return LerpV2(Left, Right, Time);
}

COVERAGE(LerpV3CPP, 1)
static inline Vec3 Lerp(Vec3 Left, Vec3 Right, Float Time)
{
    ASSERT_COVERED(LerpV3CPP);
    return LerpV3(Left, Right, Time);
}

COVERAGE(LerpV4CPP, 1)
static inline Vec4 Lerp(Vec4 Left, Vec4 Right, Float Time)
{
    ASSERT_COVERED(LerpV4CPP);
    return LerpV4(Left, Right, Time);
}

COVERAGE(ClampV2CPP, 1)
static inline Vec2 Clamp(Vec2 Value, Vec2 MinValue, Vec2 MaxValue)
{
    ASSERT_COVERED(ClampV2CPP);
    return ClampV2(Value, MinValue, MaxValue);
}

COVERAGE(ClampV3CPP, 1)
static inline Vec3 Clamp(Vec3 Value, Vec3 MinValue, Vec3 MaxValue)
{
    ASSERT_COVERED(ClampV3CPP);
    return ClampV3(Value, MinValue, MaxValue);
}

COVERAGE(ClampV4CPP, 1)
static inline Vec4 Clamp(Vec4 Value, Vec4 MinValue, Vec4 MaxValue)
{
    ASSERT_COVERED(ClampV4CPP);
    return ClampV4(Value, MinValue, MaxValue);
}

COVERAGE(IsFiniteV2CPP, 1)
static inline Bool IsFinite(Vec2 Value)
{
    ASSERT_COVERED(IsFiniteV2CPP);
    return IsFiniteV2(Value);
}

COVERAGE(IsFiniteV3CPP, 1)
static inline Bool IsFinite(Vec3 Value)
{
    ASSERT_COVERED(IsFiniteV3CPP);
    return IsFiniteV3(Value);
}

COVERAGE(IsFiniteV4CPP, 1)
static inline Bool IsFinite(Vec4 Value)
{
    ASSERT_COVERED(IsFiniteV4CPP);
    return IsFiniteV4(Value);
}

COVERAGE(TransposeM2CPP, 1)
static inline Mat2 Transpose(Mat2 Matrix)
{
    ASSERT_COVERED(TransposeM2CPP);
    return TransposeM2(Matrix);
}

COVERAGE(TransposeM3CPP, 1)
static inline Mat3 Transpose(Mat3 Matrix)
{
    ASSERT_COVERED(TransposeM3CPP);
    return TransposeM3(Matrix);
}

COVERAGE(TransposeM4CPP, 1)
static inline Mat4 Transpose(Mat4 Matrix)
{
    ASSERT_COVERED(TransposeM4CPP);
    return TransposeM4(Matrix);
}

COVERAGE(DeterminantM2CPP, 1)
static inline Float Determinant(Mat2 Matrix)
{
    ASSERT_COVERED(DeterminantM2CPP);
    return DeterminantM2(Matrix);
}

COVERAGE(DeterminantM3CPP, 1)
static inline Float Determinant(Mat3 Matrix)
{
    ASSERT_COVERED(DeterminantM3CPP);
    return DeterminantM3(Matrix);
}

COVERAGE(DeterminantM4CPP, 1)
static inline Float Determinant(Mat4 Matrix)
{
    ASSERT_COVERED(DeterminantM4CPP);
    return DeterminantM4(Matrix);
}

COVERAGE(InvGeneralM2CPP, 1)
static inline Mat2 InvGeneral(Mat2 Matrix)
{
    ASSERT_COVERED(InvGeneralM2CPP);
    return InvGeneralM2(Matrix);
}

COVERAGE(InvGeneralM3CPP, 1)
static inline Mat3 InvGeneral(Mat3 Matrix)
{
    ASSERT_COVERED(InvGeneralM3CPP);
    return InvGeneralM3(Matrix);
}

COVERAGE(InvGeneralM4CPP, 1)
static inline Mat4 InvGeneral(Mat4 Matrix)
{
    ASSERT_COVERED(InvGeneralM4CPP);
    return InvGeneralM4(Matrix);
}

COVERAGE(DotQCPP, 1)
static inline Float Dot(Quat QuatOne, Quat QuatTwo)
{
    ASSERT_COVERED(DotQCPP);
    return DotQ(QuatOne, QuatTwo);
}

COVERAGE(AddV2CPP, 1)
static inline Vec2 Add(Vec2 Left, Vec2 Right)
{
    ASSERT_COVERED(AddV2CPP);
    return AddV2(Left, Right);
}

COVERAGE(AddV3CPP, 1)
static inline Vec3 Add(Vec3 Left, Vec3 Right)
{
    ASSERT_COVERED(AddV3CPP);
    return AddV3(Left, Right);
}

COVERAGE(AddV4CPP, 1)
static inline Vec4 Add(Vec4 Left, Vec4 Right)
{
    ASSERT_COVERED(AddV4CPP);
    return AddV4(Left, Right);
}

COVERAGE(AddM2CPP, 1)
static inline Mat2 Add(Mat2 Left, Mat2 Right)
{
    ASSERT_COVERED(AddM2CPP);
    return AddM2(Left, Right);
}

COVERAGE(AddM3CPP, 1)
static inline Mat3 Add(Mat3 Left, Mat3 Right)
{
    ASSERT_COVERED(AddM3CPP);
    return AddM3(Left, Right);
}

COVERAGE(AddM4CPP, 1)
static inline Mat4 Add(Mat4 Left, Mat4 Right)
{
    ASSERT_COVERED(AddM4CPP);
    return AddM4(Left, Right);
}

COVERAGE(AddQCPP, 1)
static inline Quat Add(Quat Left, Quat Right)
{
    ASSERT_COVERED(AddQCPP);
    return AddQ(Left, Right);
}

COVERAGE(SubV2CPP, 1)
static inline Vec2 Sub(Vec2 Left, Vec2 Right)
{
    ASSERT_COVERED(SubV2CPP);
    return SubV2(Left, Right);
}

COVERAGE(SubV3CPP, 1)
static inline Vec3 Sub(Vec3 Left, Vec3 Right)
{
    ASSERT_COVERED(SubV3CPP);
    return SubV3(Left, Right);
}

COVERAGE(SubV4CPP, 1)
static inline Vec4 Sub(Vec4 Left, Vec4 Right)
{
    ASSERT_COVERED(SubV4CPP);
    return SubV4(Left, Right);
}

COVERAGE(SubM2CPP, 1)
static inline Mat2 Sub(Mat2 Left, Mat2 Right)
{
    ASSERT_COVERED(SubM2CPP);
    return SubM2(Left, Right);
}

COVERAGE(SubM3CPP, 1)
static inline Mat3 Sub(Mat3 Left, Mat3 Right)
{
    ASSERT_COVERED(SubM3CPP);
    return SubM3(Left, Right);
}

COVERAGE(SubM4CPP, 1)
static inline Mat4 Sub(Mat4 Left, Mat4 Right)
{
    ASSERT_COVERED(SubM4CPP);
    return SubM4(Left, Right);
}

COVERAGE(SubQCPP, 1)
static inline Quat Sub(Quat Left, Quat Right)
{
    ASSERT_COVERED(SubQCPP);
    return SubQ(Left, Right);
}

COVERAGE(MulV2CPP, 1)
static inline Vec2 Mul(Vec2 Left, Vec2 Right)
{
    ASSERT_COVERED(MulV2CPP);
    return MulV2(Left, Right);
}

COVERAGE(MulV2FCPP, 1)
static inline Vec2 Mul(Vec2 Left, Float Right)
{
    ASSERT_COVERED(MulV2FCPP);
    return MulV2F(Left, Right);
}

COVERAGE(MulV3CPP, 1)
static inline Vec3 Mul(Vec3 Left, Vec3 Right)
{
    ASSERT_COVERED(MulV3CPP);
    return MulV3(Left, Right);
}

COVERAGE(MulV3FCPP, 1)
static inline Vec3 Mul(Vec3 Left, Float Right)
{
    ASSERT_COVERED(MulV3FCPP);
    return MulV3F(Left, Right);
}

COVERAGE(MulV4CPP, 1)
static inline Vec4 Mul(Vec4 Left, Vec4 Right)
{
    ASSERT_COVERED(MulV4CPP);
    return MulV4(Left, Right);
}

COVERAGE(MulV4FCPP, 1)
static inline Vec4 Mul(Vec4 Left, Float Right)
{
    ASSERT_COVERED(MulV4FCPP);
    return MulV4F(Left, Right);
}

COVERAGE(MulM2CPP, 1)
static inline Mat2 Mul(Mat2 Left, Mat2 Right)
{
    ASSERT_COVERED(MulM2CPP);
    return MulM2(Left, Right);
}

COVERAGE(MulM3CPP, 1)
static inline Mat3 Mul(Mat3 Left, Mat3 Right)
{
    ASSERT_COVERED(MulM3CPP);
    return MulM3(Left, Right);
}

COVERAGE(MulM4CPP, 1)
static inline Mat4 Mul(Mat4 Left, Mat4 Right)
{
    ASSERT_COVERED(MulM4CPP);
    return MulM4(Left, Right);
}

COVERAGE(MulM2FCPP, 1)
static inline Mat2 Mul(Mat2 Left, Float Right)
{
    ASSERT_COVERED(MulM2FCPP);
    return MulM2F(Left, Right);
}

COVERAGE(MulM3FCPP, 1)
static inline Mat3 Mul(Mat3 Left, Float Right)
{
    ASSERT_COVERED(MulM3FCPP);
    return MulM3F(Left, Right);
}

COVERAGE(MulM4FCPP, 1)
static inline Mat4 Mul(Mat4 Left, Float Right)
{
    ASSERT_COVERED(MulM4FCPP);
    return MulM4F(Left, Right);
}

COVERAGE(MulM2V2CPP, 1)
static inline Vec2 Mul(Mat2 Matrix, Vec2 Vector)
{
    ASSERT_COVERED(MulM2V2CPP);
    return MulM2V2(Matrix, Vector);
}

COVERAGE(MulM3V3CPP, 1)
static inline Vec3 Mul(Mat3 Matrix, Vec3 Vector)
{
    ASSERT_COVERED(MulM3V3CPP);
    return MulM3V3(Matrix, Vector);
}

COVERAGE(MulM4V4CPP, 1)
static inline Vec4 Mul(Mat4 Matrix, Vec4 Vector)
{
    ASSERT_COVERED(MulM4V4CPP);
    return MulM4V4(Matrix, Vector);
}

COVERAGE(MulQCPP, 1)
static inline Quat Mul(Quat Left, Quat Right)
{
    ASSERT_COVERED(MulQCPP);
    return MulQ(Left, Right);
}

COVERAGE(MulQFCPP, 1)
static inline Quat Mul(Quat Left, Float Right)
{
    ASSERT_COVERED(MulQFCPP);
    return MulQF(Left, Right);
}

COVERAGE(DivV2CPP, 1)
static inline Vec2 Div(Vec2 Left, Vec2 Right)
{
    ASSERT_COVERED(DivV2CPP);
    return DivV2(Left, Right);
}

COVERAGE(DivV2FCPP, 1)
static inline Vec2 Div(Vec2 Left, Float Right)
{
    ASSERT_COVERED(DivV2FCPP);
    return DivV2F(Left, Right);
}

COVERAGE(DivV3CPP, 1)
static inline Vec3 Div(Vec3 Left, Vec3 Right)
{
    ASSERT_COVERED(DivV3CPP);
    return DivV3(Left, Right);
}

COVERAGE(DivV3FCPP, 1)
static inline Vec3 Div(Vec3 Left, Float Right)
{
    ASSERT_COVERED(DivV3FCPP);
    return DivV3F(Left, Right);
}

COVERAGE(DivV4CPP, 1)
static inline Vec4 Div(Vec4 Left, Vec4 Right)
{
    ASSERT_COVERED(DivV4CPP);
    return DivV4(Left, Right);
}

COVERAGE(DivV4FCPP, 1)
static inline Vec4 Div(Vec4 Left, Float Right)
{
    ASSERT_COVERED(DivV4FCPP);
    return DivV4F(Left, Right);
}

COVERAGE(DivM2FCPP, 1)
static inline Mat2 Div(Mat2 Left, Float Right)
{
    ASSERT_COVERED(DivM2FCPP);
    return DivM2F(Left, Right);
}

COVERAGE(DivM3FCPP, 1)
static inline Mat3 Div(Mat3 Left, Float Right)
{
    ASSERT_COVERED(DivM3FCPP);
    return DivM3F(Left, Right);
}

COVERAGE(DivM4FCPP, 1)
static inline Mat4 Div(Mat4 Left, Float Right)
{
    ASSERT_COVERED(DivM4FCPP);
    return DivM4F(Left, Right);
}

COVERAGE(DivQFCPP, 1)
static inline Quat Div(Quat Left, Float Right)
{
    ASSERT_COVERED(DivQFCPP);
    return DivQF(Left, Right);
}

COVERAGE(EqV2CPP, 1)
static inline Bool Eq(Vec2 Left, Vec2 Right)
{
    ASSERT_COVERED(EqV2CPP);
    return EqV2(Left, Right);
}

COVERAGE(EqV3CPP, 1)
static inline Bool Eq(Vec3 Left, Vec3 Right)
{
    ASSERT_COVERED(EqV3CPP);
    return EqV3(Left, Right);
}

COVERAGE(EqV4CPP, 1)
static inline Bool Eq(Vec4 Left, Vec4 Right)
{
    ASSERT_COVERED(EqV4CPP);
    return EqV4(Left, Right);
}

COVERAGE(AddV2Op, 1)
static inline Vec2 operator+(Vec2 Left, Vec2 Right)
{
    ASSERT_COVERED(AddV2Op);
    return AddV2(Left, Right);
}

COVERAGE(AddV3Op, 1)
static inline Vec3 operator+(Vec3 Left, Vec3 Right)
{
    ASSERT_COVERED(AddV3Op);
    return AddV3(Left, Right);
}

COVERAGE(AddV4Op, 1)
static inline Vec4 operator+(Vec4 Left, Vec4 Right)
{
    ASSERT_COVERED(AddV4Op);
    return AddV4(Left, Right);
}

COVERAGE(AddM2Op, 1)
static inline Mat2 operator+(Mat2 Left, Mat2 Right)
{
    ASSERT_COVERED(AddM2Op);
    return AddM2(Left, Right);
}

COVERAGE(AddM3Op, 1)
static inline Mat3 operator+(Mat3 Left, Mat3 Right)
{
    ASSERT_COVERED(AddM3Op);
    return AddM3(Left, Right);
}

COVERAGE(AddM4Op, 1)
static inline Mat4 operator+(Mat4 Left, Mat4 Right)
{
    ASSERT_COVERED(AddM4Op);
    return AddM4(Left, Right);
}

COVERAGE(AddQOp, 1)
static inline Quat operator+(Quat Left, Quat Right)
{
    ASSERT_COVERED(AddQOp);
    return AddQ(Left, Right);
}

COVERAGE(SubV2Op, 1)
static inline Vec2 operator-(Vec2 Left, Vec2 Right)
{
    ASSERT_COVERED(SubV2Op);
    return SubV2(Left, Right);
}

COVERAGE(SubV3Op, 1)
static inline Vec3 operator-(Vec3 Left, Vec3 Right)
{
    ASSERT_COVERED(SubV3Op);
    return SubV3(Left, Right);
}

COVERAGE(SubV4Op, 1)
static inline Vec4 operator-(Vec4 Left, Vec4 Right)
{
    ASSERT_COVERED(SubV4Op);
    return SubV4(Left, Right);
}

COVERAGE(SubM2Op, 1)
static inline Mat2 operator-(Mat2 Left, Mat2 Right)
{
    ASSERT_COVERED(SubM2Op);
    return SubM2(Left, Right);
}

COVERAGE(SubM3Op, 1)
static inline Mat3 operator-(Mat3 Left, Mat3 Right)
{
    ASSERT_COVERED(SubM3Op);
    return SubM3(Left, Right);
}

COVERAGE(SubM4Op, 1)
static inline Mat4 operator-(Mat4 Left, Mat4 Right)
{
    ASSERT_COVERED(SubM4Op);
    return SubM4(Left, Right);
}

COVERAGE(SubQOp, 1)
static inline Quat operator-(Quat Left, Quat Right)
{
    ASSERT_COVERED(SubQOp);
    return SubQ(Left, Right);
}

COVERAGE(MulV2Op, 1)
static inline Vec2 operator*(Vec2 Left, Vec2 Right)
{
    ASSERT_COVERED(MulV2Op);
    return MulV2(Left, Right);
}

COVERAGE(MulV3Op, 1)
static inline Vec3 operator*(Vec3 Left, Vec3 Right)
{
    ASSERT_COVERED(MulV3Op);
    return MulV3(Left, Right);
}

COVERAGE(MulV4Op, 1)
static inline Vec4 operator*(Vec4 Left, Vec4 Right)
{
    ASSERT_COVERED(MulV4Op);
    return MulV4(Left, Right);
}

COVERAGE(MulM2Op, 1)
static inline Mat2 operator*(Mat2 Left, Mat2 Right)
{
    ASSERT_COVERED(MulM2Op);
    return MulM2(Left, Right);
}

COVERAGE(MulM3Op, 1)
static inline Mat3 operator*(Mat3 Left, Mat3 Right)
{
    ASSERT_COVERED(MulM3Op);
    return MulM3(Left, Right);
}

COVERAGE(MulM4Op, 1)
static inline Mat4 operator*(Mat4 Left, Mat4 Right)
{
    ASSERT_COVERED(MulM4Op);
    return MulM4(Left, Right);
}

COVERAGE(MulQOp, 1)
static inline Quat operator*(Quat Left, Quat Right)
{
    ASSERT_COVERED(MulQOp);
    return MulQ(Left, Right);
}

COVERAGE(MulV2FOp, 1)
static inline Vec2 operator*(Vec2 Left, Float Right)
{
    ASSERT_COVERED(MulV2FOp);
    return MulV2F(Left, Right);
}

COVERAGE(MulV3FOp, 1)
static inline Vec3 operator*(Vec3 Left, Float Right)
{
    ASSERT_COVERED(MulV3FOp);
    return MulV3F(Left, Right);
}

COVERAGE(MulV4FOp, 1)
static inline Vec4 operator*(Vec4 Left, Float Right)
{
    ASSERT_COVERED(MulV4FOp);
    return MulV4F(Left, Right);
}

COVERAGE(MulM2FOp, 1)
static inline Mat2 operator*(Mat2 Left, Float Right)
{
    ASSERT_COVERED(MulM2FOp);
    return MulM2F(Left, Right);
}

COVERAGE(MulM3FOp, 1)
static inline Mat3 operator*(Mat3 Left, Float Right)
{
    ASSERT_COVERED(MulM3FOp);
    return MulM3F(Left, Right);
}

COVERAGE(MulM4FOp, 1)
static inline Mat4 operator*(Mat4 Left, Float Right)
{
    ASSERT_COVERED(MulM4FOp);
    return MulM4F(Left, Right);
}

COVERAGE(MulQFOp, 1)
static inline Quat operator*(Quat Left, Float Right)
{
    ASSERT_COVERED(MulQFOp);
    return MulQF(Left, Right);
}

COVERAGE(MulV2FOpLeft, 1)
static inline Vec2 operator*(Float Left, Vec2 Right)
{
    ASSERT_COVERED(MulV2FOpLeft);
    return MulV2F(Right, Left);
}

COVERAGE(MulV3FOpLeft, 1)
static inline Vec3 operator*(Float Left, Vec3 Right)
{
    ASSERT_COVERED(MulV3FOpLeft);
    return MulV3F(Right, Left);
}

COVERAGE(MulV4FOpLeft, 1)
static inline Vec4 operator*(Float Left, Vec4 Right)
{
    ASSERT_COVERED(MulV4FOpLeft);
    return MulV4F(Right, Left);
}

COVERAGE(MulM2FOpLeft, 1)
static inline Mat2 operator*(Float Left, Mat2 Right)
{
    ASSERT_COVERED(MulM2FOpLeft);
    return MulM2F(Right, Left);
}

COVERAGE(MulM3FOpLeft, 1)
static inline Mat3 operator*(Float Left, Mat3 Right)
{
    ASSERT_COVERED(MulM3FOpLeft);
    return MulM3F(Right, Left);
}

COVERAGE(MulM4FOpLeft, 1)
static inline Mat4 operator*(Float Left, Mat4 Right)
{
    ASSERT_COVERED(MulM4FOpLeft);
    return MulM4F(Right, Left);
}

COVERAGE(MulQFOpLeft, 1)
static inline Quat operator*(Float Left, Quat Right)
{
    ASSERT_COVERED(MulQFOpLeft);
    return MulQF(Right, Left);
}

COVERAGE(MulM2V2Op, 1)
static inline Vec2 operator*(Mat2 Matrix, Vec2 Vector)
{
    ASSERT_COVERED(MulM2V2Op);
    return MulM2V2(Matrix, Vector);
}

COVERAGE(MulM3V3Op, 1)
static inline Vec3 operator*(Mat3 Matrix, Vec3 Vector)
{
    ASSERT_COVERED(MulM3V3Op);
    return MulM3V3(Matrix, Vector);
}

COVERAGE(MulM4V4Op, 1)
static inline Vec4 operator*(Mat4 Matrix, Vec4 Vector)
{
    ASSERT_COVERED(MulM4V4Op);
    return MulM4V4(Matrix, Vector);
}

COVERAGE(DivV2Op, 1)
static inline Vec2 operator/(Vec2 Left, Vec2 Right)
{
    ASSERT_COVERED(DivV2Op);
    return DivV2(Left, Right);
}

COVERAGE(DivV3Op, 1)
static inline Vec3 operator/(Vec3 Left, Vec3 Right)
{
    ASSERT_COVERED(DivV3Op);
    return DivV3(Left, Right);
}

COVERAGE(DivV4Op, 1)
static inline Vec4 operator/(Vec4 Left, Vec4 Right)
{
    ASSERT_COVERED(DivV4Op);
    return DivV4(Left, Right);
}

COVERAGE(DivV2FOp, 1)
static inline Vec2 operator/(Vec2 Left, Float Right)
{
    ASSERT_COVERED(DivV2FOp);
    return DivV2F(Left, Right);
}

COVERAGE(DivV3FOp, 1)
static inline Vec3 operator/(Vec3 Left, Float Right)
{
    ASSERT_COVERED(DivV3FOp);
    return DivV3F(Left, Right);
}

COVERAGE(DivV4FOp, 1)
static inline Vec4 operator/(Vec4 Left, Float Right)
{
    ASSERT_COVERED(DivV4FOp);
    return DivV4F(Left, Right);
}

COVERAGE(DivM4FOp, 1)
static inline Mat4 operator/(Mat4 Left, Float Right)
{
    ASSERT_COVERED(DivM4FOp);
    return DivM4F(Left, Right);
}

COVERAGE(DivM3FOp, 1)
static inline Mat3 operator/(Mat3 Left, Float Right)
{
    ASSERT_COVERED(DivM3FOp);
    return DivM3F(Left, Right);
}

COVERAGE(DivM2FOp, 1)
static inline Mat2 operator/(Mat2 Left, Float Right)
{
    ASSERT_COVERED(DivM2FOp);
    return DivM2F(Left, Right);
}

COVERAGE(DivQFOp, 1)
static inline Quat operator/(Quat Left, Float Right)
{
    ASSERT_COVERED(DivQFOp);
    return DivQF(Left, Right);
}

COVERAGE(AddV2Assign, 1)
static inline Vec2 &operator+=(Vec2 &Left, Vec2 Right)
{
    ASSERT_COVERED(AddV2Assign);
    return Left = Left + Right;
}

COVERAGE(AddV3Assign, 1)
static inline Vec3 &operator+=(Vec3 &Left, Vec3 Right)
{
    ASSERT_COVERED(AddV3Assign);
    return Left = Left + Right;
}

COVERAGE(AddV4Assign, 1)
static inline Vec4 &operator+=(Vec4 &Left, Vec4 Right)
{
    ASSERT_COVERED(AddV4Assign);
    return Left = Left + Right;
}

COVERAGE(AddM2Assign, 1)
static inline Mat2 &operator+=(Mat2 &Left, Mat2 Right)
{
    ASSERT_COVERED(AddM2Assign);
    return Left = Left + Right;
}

COVERAGE(AddM3Assign, 1)
static inline Mat3 &operator+=(Mat3 &Left, Mat3 Right)
{
    ASSERT_COVERED(AddM3Assign);
    return Left = Left + Right;
}

COVERAGE(AddM4Assign, 1)
static inline Mat4 &operator+=(Mat4 &Left, Mat4 Right)
{
    ASSERT_COVERED(AddM4Assign);
    return Left = Left + Right;
}

COVERAGE(AddQAssign, 1)
static inline Quat &operator+=(Quat &Left, Quat Right)
{
    ASSERT_COVERED(AddQAssign);
    return Left = Left + Right;
}

COVERAGE(SubV2Assign, 1)
static inline Vec2 &operator-=(Vec2 &Left, Vec2 Right)
{
    ASSERT_COVERED(SubV2Assign);
    return Left = Left - Right;
}

COVERAGE(SubV3Assign, 1)
static inline Vec3 &operator-=(Vec3 &Left, Vec3 Right)
{
    ASSERT_COVERED(SubV3Assign);
    return Left = Left - Right;
}

COVERAGE(SubV4Assign, 1)
static inline Vec4 &operator-=(Vec4 &Left, Vec4 Right)
{
    ASSERT_COVERED(SubV4Assign);
    return Left = Left - Right;
}

COVERAGE(SubM2Assign, 1)
static inline Mat2 &operator-=(Mat2 &Left, Mat2 Right)
{
    ASSERT_COVERED(SubM2Assign);
    return Left = Left - Right;
}

COVERAGE(SubM3Assign, 1)
static inline Mat3 &operator-=(Mat3 &Left, Mat3 Right)
{
    ASSERT_COVERED(SubM3Assign);
    return Left = Left - Right;
}

COVERAGE(SubM4Assign, 1)
static inline Mat4 &operator-=(Mat4 &Left, Mat4 Right)
{
    ASSERT_COVERED(SubM4Assign);
    return Left = Left - Right;
}

COVERAGE(SubQAssign, 1)
static inline Quat &operator-=(Quat &Left, Quat Right)
{
    ASSERT_COVERED(SubQAssign);
    return Left = Left - Right;
}

COVERAGE(MulV2Assign, 1)
static inline Vec2 &operator*=(Vec2 &Left, Vec2 Right)
{
    ASSERT_COVERED(MulV2Assign);
    return Left = Left * Right;
}

COVERAGE(MulV3Assign, 1)
static inline Vec3 &operator*=(Vec3 &Left, Vec3 Right)
{
    ASSERT_COVERED(MulV3Assign);
    return Left = Left * Right;
}

COVERAGE(MulV4Assign, 1)
static inline Vec4 &operator*=(Vec4 &Left, Vec4 Right)
{
    ASSERT_COVERED(MulV4Assign);
    return Left = Left * Right;
}

COVERAGE(MulV2FAssign, 1)
static inline Vec2 &operator*=(Vec2 &Left, Float Right)
{
    ASSERT_COVERED(MulV2FAssign);
    return Left = Left * Right;
}

COVERAGE(MulV3FAssign, 1)
static inline Vec3 &operator*=(Vec3 &Left, Float Right)
{
    ASSERT_COVERED(MulV3FAssign);
    return Left = Left * Right;
}

COVERAGE(MulV4FAssign, 1)
static inline Vec4 &operator*=(Vec4 &Left, Float Right)
{
    ASSERT_COVERED(MulV4FAssign);
    return Left = Left * Right;
}

COVERAGE(MulM2FAssign, 1)
static inline Mat2 &operator*=(Mat2 &Left, Float Right)
{
    ASSERT_COVERED(MulM2FAssign);
    return Left = Left * Right;
}

COVERAGE(MulM3FAssign, 1)
static inline Mat3 &operator*=(Mat3 &Left, Float Right)
{
    ASSERT_COVERED(MulM3FAssign);
    return Left = Left * Right;
}

COVERAGE(MulM4FAssign, 1)
static inline Mat4 &operator*=(Mat4 &Left, Float Right)
{
    ASSERT_COVERED(MulM4FAssign);
    return Left = Left * Right;
}

COVERAGE(MulQFAssign, 1)
static inline Quat &operator*=(Quat &Left, Float Right)
{
    ASSERT_COVERED(MulQFAssign);
    return Left = Left * Right;
}

COVERAGE(DivV2Assign, 1)
static inline Vec2 &operator/=(Vec2 &Left, Vec2 Right)
{
    ASSERT_COVERED(DivV2Assign);
    return Left = Left / Right;
}

COVERAGE(DivV3Assign, 1)
static inline Vec3 &operator/=(Vec3 &Left, Vec3 Right)
{
    ASSERT_COVERED(DivV3Assign);
    return Left = Left / Right;
}

COVERAGE(DivV4Assign, 1)
static inline Vec4 &operator/=(Vec4 &Left, Vec4 Right)
{
    ASSERT_COVERED(DivV4Assign);
    return Left = Left / Right;
}

COVERAGE(DivV2FAssign, 1)
static inline Vec2 &operator/=(Vec2 &Left, Float Right)
{
    ASSERT_COVERED(DivV2FAssign);
    return Left = Left / Right;
}

COVERAGE(DivV3FAssign, 1)
static inline Vec3 &operator/=(Vec3 &Left, Float Right)
{
    ASSERT_COVERED(DivV3FAssign);
    return Left = Left / Right;
}

COVERAGE(DivV4FAssign, 1)
static inline Vec4 &operator/=(Vec4 &Left, Float Right)
{
    ASSERT_COVERED(DivV4FAssign);
    return Left = Left / Right;
}

COVERAGE(DivM4FAssign, 1)
static inline Mat4 &operator/=(Mat4 &Left, Float Right)
{
    ASSERT_COVERED(DivM4FAssign);
    return Left = Left / Right;
}

COVERAGE(DivQFAssign, 1)
static inline Quat &operator/=(Quat &Left, Float Right)
{
    ASSERT_COVERED(DivQFAssign);
    return Left = Left / Right;
}

COVERAGE(EqV2Op, 1)
static inline Bool operator==(Vec2 Left, Vec2 Right)
{
    ASSERT_COVERED(EqV2Op);
    return EqV2(Left, Right);
}

COVERAGE(EqV3Op, 1)
static inline Bool operator==(Vec3 Left, Vec3 Right)
{
    ASSERT_COVERED(EqV3Op);
    return EqV3(Left, Right);
}

COVERAGE(EqV4Op, 1)
static inline Bool operator==(Vec4 Left, Vec4 Right)
{
    ASSERT_COVERED(EqV4Op);
    return EqV4(Left, Right);
}

COVERAGE(EqV2OpNot, 1)
static inline Bool operator!=(Vec2 Left, Vec2 Right)
{
    ASSERT_COVERED(EqV2OpNot);
    return !EqV2(Left, Right);
}

COVERAGE(EqV3OpNot, 1)
static inline Bool operator!=(Vec3 Left, Vec3 Right)
{
    ASSERT_COVERED(EqV3OpNot);
    return !EqV3(Left, Right);
}

COVERAGE(EqV4OpNot, 1)
static inline Bool operator!=(Vec4 Left, Vec4 Right)
{
    ASSERT_COVERED(EqV4OpNot);
    return !EqV4(Left, Right);
}

COVERAGE(UnaryMinusV2, 1)
static inline Vec2 operator-(Vec2 In)
{
    ASSERT_COVERED(UnaryMinusV2);

    Vec2 Result;
    Result.X = -In.X;
    Result.Y = -In.Y;

    return Result;
}

COVERAGE(UnaryMinusV3, 1)
static inline Vec3 operator-(Vec3 In)
{
    ASSERT_COVERED(UnaryMinusV3);

    Vec3 Result;
    Result.X = -In.X;
    Result.Y = -In.Y;
    Result.Z = -In.Z;

    return Result;
}

COVERAGE(UnaryMinusV4, 1)
static inline Vec4 operator-(Vec4 In)
{
    ASSERT_COVERED(UnaryMinusV4);

    Vec4 Result;
#if HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_xor_ps(In.SSE, _mm_set1_ps(-0.0f));
#else
    Result.X = -In.X;
    Result.Y = -In.Y;
    Result.Z = -In.Z;
    Result.W = -In.W;
#endif

    return Result;
}

#endif /* __cplusplus*/

#ifdef HANDMADE_MATH__USE_C11_GENERICS
#define Add(A, B) _Generic((A), \
    Vec2: AddV2,                \
    Vec3: AddV3,                \
    Vec4: AddV4,                \
    Mat2: AddM2,                \
    Mat3: AddM3,                \
    Mat4: AddM4,                \
    Quat: AddQ)(A, B)

#define Sub(A, B) _Generic((A), \
    Vec2: SubV2,                \
    Vec3: SubV3,                \
    Vec4: SubV4,                \
    Mat2: SubM2,                \
    Mat3: SubM3,                \
    Mat4: SubM4,                \
    Quat: SubQ)(A, B)

#define Mul(A, B) _Generic((B), \
    Float: _Generic((A),        \
        Vec2: MulV2F,               \
        Vec3: MulV3F,               \
        Vec4: MulV4F,               \
        Mat2: MulM2F,               \
        Mat3: MulM3F,               \
        Mat4: MulM4F,               \
        Quat: MulQF),               \
        Mat2: MulM2,                \
        Mat3: MulM3,                \
        Mat4: MulM4,                \
        Quat: MulQ,                 \
    default: _Generic((A),      \
        Vec2: MulV2,                \
        Vec3: MulV3,                \
        Vec4: MulV4,                \
        Mat2: MulM2V2,              \
        Mat3: MulM3V3,              \
        Mat4: MulM4V4))(A, B)

#define Div(A, B) _Generic((B), \
    Float: _Generic((A),        \
        Mat2: DivM2F,               \
        Mat3: DivM3F,               \
        Mat4: DivM4F,               \
        Vec2: DivV2F,               \
        Vec3: DivV3F,               \
        Vec4: DivV4F,               \
        Quat: DivQF),               \
    default: _Generic((A),      \
        Vec2: DivV2,                \
        Vec3: DivV3,                \
        Vec4: DivV4))(A, B)

#define Len(A) _Generic((A), \
    Vec2: LenV2,             \
    Vec3: LenV3,             \
    Vec4: LenV4)(A)

#define LenSqr(A) _Generic((A), \
    Vec2: LenSqrV2,             \
    Vec3: LenSqrV3,             \
    Vec4: LenSqrV4)(A)

#define Norm(A) _Generic((A), \
    Vec2: NormV2,             \
    Vec3: NormV3,             \
    Vec4: NormV4)(A)

#define Dot(A, B) _Generic((A), \
    Vec2: DotV2,                \
    Vec3: DotV3,                \
    Vec4: DotV4)(A, B)

#define Min(A, B) _Generic((A), \
    Float: Min,                 \
    Vec2: MinV2,                \
    Vec3: MinV3,                \
    Vec4: MinV4)(A, B)

#define Max(A, B) _Generic((A), \
    Float: Max,                 \
    Vec2: MaxV2,                \
    Vec3: MaxV3,                \
    Vec4: MaxV4)(A, B)

#define Abs(V) _Generic((V), \
    Float: Abs,              \
    Vec2: AbsV2,             \
    Vec3: AbsV3,             \
    Vec4: AbsV4)(V)

#define Lerp(A, B, T) _Generic((A), \
    Float: Lerp,                    \
    Vec2: LerpV2,                   \
    Vec3: LerpV3,                   \
    Vec4: LerpV4)(A, B, T)

#define Clamp(V, MinValue, MaxValue) _Generic((V), \
    Float: Clamp,                                  \
    Vec2: ClampV2,                                 \
    Vec3: ClampV3,                                 \
    Vec4: ClampV4)(V, MinValue, MaxValue)

#define IsFinite(V) _Generic((V), \
    Float: IsFinite,              \
    Vec2: IsFiniteV2,             \
    Vec3: IsFiniteV3,             \
    Vec4: IsFiniteV4)(V)

#define Eq(A, B) _Generic((A), \
    Vec2: EqV2,                \
    Vec3: EqV3,                \
    Vec4: EqV4)(A, B)

#define Transpose(M) _Generic((M), \
    Mat2: TransposeM2,             \
    Mat3: TransposeM3,             \
    Mat4: TransposeM4)(M)

#define Determinant(M) _Generic((M), \
    Mat2: DeterminantM2,             \
    Mat3: DeterminantM3,             \
    Mat4: DeterminantM4)(M)

#define InvGeneral(M) _Generic((M), \
    Mat2: InvGeneralM2,             \
    Mat3: InvGeneralM3,             \
    Mat4: InvGeneralM4)(M)

#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif /* HANDMADE_MATH_H */
