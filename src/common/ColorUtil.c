#include <SDL3/SDL.h>

#include "stb_ds.h"
#include "stb_image_write.h"

#include "ColorUtil.h"
#include "HandmadeMath.h"

int GradientColorPointCompare(const void* A, const void* B)
{
	GradientColorPoint* ColorA = (GradientColorPoint*)A;
	GradientColorPoint* ColorB = (GradientColorPoint*)B;

	if (ColorA->Position < ColorB->Position) {
		return -1;
	} else if (ColorA->Position > ColorB->Position) {
		return 1;
	} else {
		return 0;
	}
}

// Assumes color points are already sorted
Vec4 _ColorGradientSample(GradientColorPoint* ColorPoints, uint32 NumColorPoints, float32 Position)
{
	Position = Clamp(Position, 0.0f, 1.0f);

	GradientColorPoint *Left = NULL, *Right = NULL;
	for (uint32 ColorPointIndex = 0; ColorPointIndex < NumColorPoints; ColorPointIndex++) {
		GradientColorPoint* Current = &ColorPoints[ColorPointIndex];
		if (Current->Position <= Position && (Left == NULL || Current->Position > Left->Position)) {
			Left = Current;
		} else if (Current->Position >= Position) {
			Right = Current;
			break;
		}
	}

	ASSERT(Left && Right);

	float32 Delta = Right->Position - Left->Position;
	float32 LocalPosition = Position - Left->Position;
	float32 LocalNormPosition = (Delta != 0.0f) ? LocalPosition / Delta : 0.0f;

	Vec4 Color = Lerp(Left->Color, Right->Color, LocalNormPosition);

	return Color;
}

bool ColorGradientFromColorPoints(
	GradientColorPoint* ColorPoints,
	uint32 NumColorPoints,
	uint32 GradientWidth,
	ColorU8* OutGradient)
{
	if (NumColorPoints < 2) {
		return false;
	}

	if (GradientWidth < 1) {
		return false;
	}

	SDL_qsort(ColorPoints, NumColorPoints, sizeof(*ColorPoints), GradientColorPointCompare);

	// Ensure first and last color points are at positions 0 and 1 respectively
	ColorPoints[0].Position = 0.0f;
	ColorPoints[NumColorPoints - 1].Position = 1.0f;

	for (uint32 GradientX = 0; GradientX < GradientWidth; GradientX++) {
		float32 GradientNormX = (float32)GradientX / GradientWidth;
		Vec4 Color = _ColorGradientSample(ColorPoints, NumColorPoints, GradientNormX);
		OutGradient[GradientX] = ColorU8FromVec4(Color);
	}

	return true;
}

void ColorV4ToBytes(Vec4 Color, uint8* R, uint8* G, uint8* B, uint8* A)
{
	if (R != NULL) *R = (uint8)round(Color.R * 255.0f);
	if (G != NULL) *G = (uint8)round(Color.G * 255.0f);
	if (B != NULL) *B = (uint8)round(Color.B * 255.0f);
	if (A != NULL) *A = (uint8)round(Color.A * 255.0f);
}

ColorU8 ColorU8FromVec4(Vec4 Color)
{
	ColorU8 Result;
	Result.R = (uint8)round(Color.R * 255.0f);
	Result.G = (uint8)round(Color.G * 255.0f);
	Result.B = (uint8)round(Color.B * 255.0f);
	Result.A = (uint8)round(Color.A * 255.0f);
	return Result;
}

ColorU8 ColorU8FromColorU32(uint32 Color)
{
	ColorU8 Result;
	Result.R = Color & 0xFF;
	Result.G = Color >> 8;
	Result.B = Color >> 16;
	Result.A = Color >> 24;
	return Result;
}

ColorU8 ColorU8FromHSV(float32 H, float32 S, float32 V, float32 A)
{
	if (S == 0.0f) {
		uint8 V8 = (uint8)(V * 255.0f);
		uint8 A8 = (uint8)(A * 255.0f);
		return (ColorU8){V8, V8, V8, A8};
	}

	H = (H - floor(H)) * 6.0f;
	int32 HI = (int32)H;
	float32 Frac = H - HI;
	float32 N0 = V * (1.0f - S);
	float32 N1 = V * (1.0f - S * Frac);
	float32 N2 = V * (1.0f - S * (1.0f - Frac));

	float32 RP = V, GP = V, BP = V;
	// clang-format off
	switch (HI)
	{
		case 0: GP = N2; BP = N0; break;
		case 1: RP = N1; BP = N0; break;
		case 2: RP = N0; BP = N2; break;
		case 3: RP = N0; GP = N1; break;
		case 4: RP = N2; GP = N0; break;
		case 5: GP = N0; BP = N1; break;
		default: unreachable(); break;
	}
	// clang-format on

	return ColorU8FromVec4(V4(RP, GP, BP, A));
}

bool ColorPointsExportToImageFile(
	const char* FileName,
	GradientColorPoint* ColorPoints,
	uint32 NumColorPoints,
	uint32 GradientWidth)
{
	bool Success = false;
	ColorU8* Colors = NULL;
	arrsetlen(Colors, GradientWidth);

	if (ColorGradientFromColorPoints(ColorPoints, NumColorPoints, GradientWidth, Colors)) {
		Success = stbi_write_png(FileName, GradientWidth, 1, 4, Colors, sizeof(*Colors) * GradientWidth);
	}

	arrfree(Colors);

	return Success;
}

bool ColorGradientExportToImageFile(const char* FileName, ColorU8* Colors, uint32 NumColors)
{
	return stbi_write_png(FileName, NumColors, 1, 4, Colors, sizeof(*Colors) * NumColors);
}