#pragma once

#include <SDL3/SDL_pixels.h>

#include "HandmadeMath.h"
#include "Types.h"

typedef struct GradientColorPoint {
	float32 Position; // 0-1
	Vec4 Color;
} GradientColorPoint;

// Assumes OutGradient is a buffer large enough to store GradientWidth elements
bool ColorGradientFromColorPoints(
	GradientColorPoint* ColorPoints,
	uint32 NumColorPoints,
	uint32 GradientWidth,
	ColorU8* OutGradient);

void ColorV4ToBytes(Vec4 Color, uint8* R, uint8* G, uint8* B, uint8* A);
ColorU8 ColorU8FromVec4(Vec4 Color);
ColorU8 ColorU8FromColorU32(uint32 Color);
ColorU8 ColorU8FromHSV(float32 H, float32 S, float32 V, float32 A);

bool ColorPointsExportToImageFile(
	const char* FileName,
	GradientColorPoint* ColorPoints,
	uint32 NumColorPoints,
	uint32 GradientWidth);

bool ColorGradientExportToImageFile(
	const char* FileName,
	ColorU8* Colors,
	uint32 NumColors);