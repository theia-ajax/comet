#include "RenderUtil.h"

#include <SDL3/SDL.h>

#include "Log.h"

const char* SelectRenderDriver(const char* RequestedRenderDriver)
{
	const char* SelectedRenderDriver = NULL;
	const char* DefaultRenderDriver = NULL;

	LogInfo("Available render drivers:");
	for (int DriverIndex = 0; DriverIndex < SDL_GetNumRenderDrivers(); DriverIndex++) {
		const char* Driver = SDL_GetRenderDriver(DriverIndex);
		LogInfo("%02d) %s", DriverIndex + 1, Driver);
		if (DriverIndex == 0) {
			DefaultRenderDriver = Driver;
		}
		if (RequestedRenderDriver && !SelectedRenderDriver && SDL_strcasecmp(RequestedRenderDriver, Driver) == 0) {
			SelectedRenderDriver = Driver;
		}
	}

	if (!RequestedRenderDriver) {
		LogInfo("No render driver name provided by config, using default '%s'", DefaultRenderDriver);
	} else if (!SelectedRenderDriver) {
		LogWarning(
			"No render driver with name '%s' provided by config could be found, using default '%s'",
			RequestedRenderDriver,
			DefaultRenderDriver);
	}

	SelectedRenderDriver = SelectedRenderDriver ? SelectedRenderDriver : DefaultRenderDriver;

	return SelectedRenderDriver;
}

void SDL_RenderAABB(SDL_Renderer* Renderer, const AABB Box)
{
	static_assert(sizeof(Vec2) == sizeof(SDL_FPoint));
	Vec2 Verts[5];
	AABBGetVertices(Box, Verts);
	Verts[4] = Verts[0];
	SDL_RenderLines(Renderer, (SDL_FPoint*)Verts, 5);
}

void SDL_RenderCircle(SDL_Renderer* Renderer, const Vec2 Center, const float32 Radius)
{
	static_assert(sizeof(Vec2) == sizeof(SDL_FPoint));
	enum { KCircleSegments = 15 };

	Vec2 Points[KCircleSegments + 1];

	for (int32 Index = 0; Index < KCircleSegments; Index++)
	{
		float32 Angle = (float32)Index / KCircleSegments;
		Points[Index].X = CosF(Angle) * Radius + Center.X;
		Points[Index].Y = SinF(Angle) * Radius + Center.Y;
	}

	Points[KCircleSegments] = Points[0];

	SDL_RenderLines(Renderer, (SDL_FPoint*)Points, KCircleSegments + 1);
}