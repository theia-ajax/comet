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
