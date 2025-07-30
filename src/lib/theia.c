#include <SDL3/SDL.h>

#include <stddef.h>
#include <stdio.h>

#include "AssetTypes.h"
#include "ColorUtil.h"
#include "Log.h"
#include "ParticleSandbox.h"
#include "RenderUtil.h"
#include "StringId.h"

#define RND_IMPLEMENTATION
#include "Random.h"

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define SOKOL_TIME_IMPL
#include <sokol_time.h>

#define LOG_CALL(x) (x), LogInfo(#x)

void Initialize(LogLevel LoggingLevel, const char* RequestedRenderDriver);
void Shutdown(void);
void StepSimulation(float32 DeltaTime);
void RenderSimulationToFile(const char* FileName);
SDL_Surface* RenderSimulationToSurface();
void DestroyRenderedSurface(void* SurfacePtr);

struct {
	SDL_Window* Window;
	SDL_Renderer* Renderer;
	SDL_Texture* RenderTexture;
	ColorU8* HeatGradient;
	ImageData ParticleImage;
	SDL_Texture* ParticleTexture;
} G;

void Initialize(LogLevel LoggingLevel, const char* RequestedRenderDriver)
{
	stm_setup();
	LoggingInitialize(LoggingLevel);
	StringIdPoolsInitialize();

	LogInfo("flags: %u", offsetof(SDL_Surface, flags));
	LogInfo("format: %u", offsetof(SDL_Surface, format));
	LogInfo("w: %u", offsetof(SDL_Surface, w));
	LogInfo("h: %u", offsetof(SDL_Surface, h));
	LogInfo("pitch: %u", offsetof(SDL_Surface, pitch));
	LogInfo("pixels: %u", offsetof(SDL_Surface, pixels));
	LogInfo("refcount: %u", offsetof(SDL_Surface, refcount));
	LogInfo("reserved: %u", offsetof(SDL_Surface, reserved));

	RandomSetSeed((uint32)SDL_GetPerformanceCounter());

	ParticleSandboxConfig Config = ParticleSandboxDefaultConfig();

	// Initialize Renderer
	{
		G.Window = SDL_CreateWindow("theia", 1480, 320, 0);
		SDL_HideWindow(G.Window);

		const char* RenderDriver = SelectRenderDriver(RequestedRenderDriver);
		G.Renderer = SDL_CreateRenderer(G.Window, RenderDriver);
		if (G.Renderer) {
			LogInfo("Created renderer with %s render driver", RenderDriver);
		} else {
			LogError("Failed to create renderer: %s", SDL_GetError());
		}
		SDL_SetRenderDrawBlendMode(G.Renderer, SDL_BLENDMODE_BLEND);

		SDL_PropertiesID RendererProperties = SDL_GetRendererProperties(G.Renderer);
		SDL_PixelFormat* RendererPixelFormats = SDL_GetPointerProperty(
			SDL_GetRendererProperties(G.Renderer),
			SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER,
			NULL);

		SDL_PixelFormat Format = SDL_PIXELFORMAT_UNKNOWN;
		if (RendererPixelFormats) {
			Format = RendererPixelFormats[0];
		}

		G.RenderTexture = SDL_CreateTexture(
			G.Renderer,
			Format,
			SDL_TEXTUREACCESS_TARGET,
			Config.Rendering.Width,
			Config.Rendering.Height);
		SDL_SetTextureScaleMode(G.RenderTexture, SDL_SCALEMODE_LINEAR);
	}

	// Heat gradient
	{
		const uint32 HeatRampSize = 1024;
		arrsetlen(G.HeatGradient, HeatRampSize);
		GradientColorPoint ColorPoints[] = {
			(GradientColorPoint){.Position = 0, .Color = V4(0, 0, 0, 1)},
			(GradientColorPoint){.Position = 0.25f, .Color = V4(151 / 255.0f, 42 / 255.0f, 68 / 255.0f, 1)},
			(GradientColorPoint){.Position = 0.6f, .Color = V4(236 / 255.0f, 49 / 255.0f, 216 / 255.0f, 1)},
			(GradientColorPoint){.Position = 1, .Color = V4(1, 1, 1, 1)},
		};
		ColorGradientFromColorPoints(ColorPoints, ARRAY_COUNT(ColorPoints), HeatRampSize, G.HeatGradient);
	}

	// Particle texture
	{
		const char* ParticleImageFileName = "assets/smallflare.png";
		if (!LoadImageData(ParticleImageFileName, &G.ParticleImage)) {
			LogError("Failed to load particle image '%s'", ParticleImageFileName);
		}

		G.ParticleTexture = SDL_CreateTextureFromSurface(G.Renderer, G.ParticleImage.Surface);

		if (!G.ParticleTexture) {
			LogError("Failed to create particle texture: '%s'", SDL_GetError());
		}
	}

	LOG_CALL(ParticleSandboxInitialize(&Config));
}

void Shutdown(void)
{
	LOG_CALL(UnloadImageData(&G.ParticleImage));
	LOG_CALL(arrfree(G.HeatGradient));
	LOG_CALL(SDL_DestroyTexture(G.RenderTexture));
	LOG_CALL(SDL_DestroyRenderer(G.Renderer));
	LOG_CALL(SDL_DestroyWindow(G.Window));

	LOG_CALL(ParticleSandboxShutdown());
	StringIdPoolsShutdown();
	LoggingShutdown();
}

void StepSimulation(float32 DeltaTime)
{
	ParticleSandboxUpdate(DeltaTime);
}

void RenderSimulationToFile(const char* FileName)
{
	LogInfo(__FUNCTION__);
	LogInfo("FileName: %s", FileName);

	SDL_Surface* RenderedSurface = RenderSimulationToSurface();

	if (!stbi_write_png(
			FileName,
			RenderedSurface->w,
			RenderedSurface->h,
			RenderedSurface->pitch / RenderedSurface->w,
			RenderedSurface->pixels,
			RenderedSurface->pitch))
	{
		LogError("Failed to write to '%s'", FileName);
	}

	SDL_DestroySurface(RenderedSurface);
}

SDL_Surface* RenderSimulationToSurface()
{
	SDL_SetRenderDrawColor(G.Renderer, 0, 0, 0, 0);
	SDL_RenderClear(G.Renderer);

	ParticleSandboxRenderToTexture(&(ParticleSandboxRenderContext){
		.HeatGradient = G.HeatGradient,
		.Renderer = G.Renderer,
		.TargetTexture = G.RenderTexture,
		.ParticleTexture = G.ParticleTexture,
	});

	if (!SDL_RenderTexture(G.Renderer, G.RenderTexture, NULL, NULL)) {
		LogError("%s", SDL_GetError());
	}

	SDL_Surface* RenderedSurface = SDL_RenderReadPixels(G.Renderer, NULL);

	return RenderedSurface;
}

void DestroyRenderedSurface(void* SurfacePtr)
{
	SDL_DestroySurface((SDL_Surface*)SurfacePtr);
}
