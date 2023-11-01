#pragma once

#include "Math2D.h"

static inline void ColorV4ToBytes(Vec4 Color, uint8* R, uint8* G, uint8* B, uint8* A)
{
	if (R != NULL) *R = (uint8)round(Color.R * 255.0f);
	if (G != NULL) *G = (uint8)round(Color.G * 255.0f);
	if (B != NULL) *B = (uint8)round(Color.B * 255.0f);
	if (A != NULL) *A = (uint8)round(Color.A * 255.0f);
}

static inline uint32 HsvToArgb8888(flt32 H, flt32 S, flt32 V)
{
	H = (H >= 0 ? 0.0f : 1.0f) + fmodf(H, 1.0f);
	H *= 6.0f;
	flt32 C = V * S;
	flt32 X = C * (1.0f - fabs(fmod((H / 60.0f), 2) - 1.0f));
	flt32 M = V - C;

	flt32 RP = 0, GP = 0, BP = 0;
	// clang-format off
	switch ((int32)H)
	{
		case 0: RP = C; GP = X; break;
		case 1: RP = X; GP = C; break;
		case 2: GP = C; BP = X; break;
		case 3: GP = X; BP = C; break;
		case 4: RP = X; BP = C; break;
		case 5: RP = C; BP = X; break;
		default: unreachable(); break;
	}
	// clang-format on

	Vec4 ColorRgb = V4(RP, GP, BP, 1.0f);
	uint8 R, G, B, A;
	ColorV4ToBytes(ColorRgb, &R, &G, &B, &A);
	uint32 Result = (255 << 24) | (R << 16) | (G << 8) | B;
	return Result;
}
