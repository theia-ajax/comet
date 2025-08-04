#include <SDL3/SDL.h>

#include <sokol_time.h>
#include <stb_ds.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include "ColorUtil.h"
#include "Log.h"
#include "ParticleSandbox.h"
#include "Random.h"
#include "RenderUtil.h"
#include "StringId.h"

#include "data.h"

#define LOG_CALL(x) (x), LogInfo(#x)

void Initialize(LogLevel LoggingLevel, const char* RequestedRenderDriver);
void Shutdown(void);
void StepSimulation(float32 DeltaTime);
void RenderSimulationToFile(const char* FileName);
SDL_Surface* RenderSimulationToSurface();
void DestroyRenderedSurface(void* SurfacePtr);
// void ParticleImageToSourceFile(void);

struct {
	SDL_Window* Window;
	SDL_Renderer* Renderer;
	SDL_Texture* RenderTexture;
	ColorU8* HeatGradient;
	// ImageData ParticleImage;
	SDL_Surface* ParticleImageSurface;
	SDL_Texture* ParticleTexture;
	SDL_Surface* BlankSurface;
	bool InitializedRenderer;
} G;

void Initialize(LogLevel LoggingLevel, const char* RequestedRenderDriver)
{
	ZERO_STRUCT(&G);

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

	RandomSetSeed(0);

	ParticleSandboxConfig Config = ParticleSandboxDefaultConfig();

	LOG_CALL(ParticleSandboxInitialize(&Config));

	G.BlankSurface = SDL_CreateSurface(Config.Rendering.Width, Config.Rendering.Height, SDL_PIXELFORMAT_BGRA32);
	SDL_memset4(G.BlankSurface->pixels, 0x00000000, Config.Rendering.Width * Config.Rendering.Height);

	// Initialize Renderer
	{
		G.Window = SDL_CreateWindow("theia", Config.Rendering.Width, Config.Rendering.Height, SDL_WINDOW_HIDDEN);

		if (!G.Window) {
			LogError("Failed to create window: %s", SDL_GetError());
			return;
		}

		const char* RenderDriver = SelectRenderDriver(RequestedRenderDriver);
		G.Renderer = SDL_CreateRenderer(G.Window, RenderDriver);
		if (G.Renderer) {
			LogInfo("Created renderer with %s render driver", RenderDriver);
		} else {
			LogError("Failed to create renderer: %s", SDL_GetError());
			return;
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

		if (!G.RenderTexture) {
			LogError("Failed to create render texture: %s", SDL_GetError());
			return;
		}
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
		// const char* ParticleImageFileName = "assets/smallflare.png";
		// if (!LoadImageData(ParticleImageFileName, &G.ParticleImage)) {
		// 	LogError("Failed to load particle image '%s'", ParticleImageFileName);
		// }

		G.ParticleImageSurface = Data_CreateImageSurface();

		G.ParticleTexture = SDL_CreateTextureFromSurface(G.Renderer, G.ParticleImageSurface);

		if (!G.ParticleTexture) {
			LogError("Failed to create particle texture: '%s'", SDL_GetError());
		}
	}

	G.InitializedRenderer = true;
}

void Shutdown(void)
{
	SDL_DestroySurface(G.BlankSurface);

	// LOG_CALL(UnloadImageData(&G.ParticleImage));
	LOG_CALL(SDL_DestroySurface(G.ParticleImageSurface));
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
	if (G.InitializedRenderer) {
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
	} else {
		return G.BlankSurface;
	}
}

void DestroyRenderedSurface(void* SurfacePtr)
{
	if ((SDL_Surface*)SurfacePtr != G.BlankSurface) {
		SDL_DestroySurface((SDL_Surface*)SurfacePtr);
	}
}

#if 0
void ParticleImageToSourceFile(void)
{
	FILE* File = fopen("src/lib/data.h", "w");

	if (!File) {
		return;
	}

	int Width = G.ParticleImage.Width;
	int Height = G.ParticleImage.Height;
	uint8* Bytes = (uint8*)G.ParticleImage.Pixels;

	fprintf(File, "#pragma once\n\n");
	fprintf(File, "const int KImageWidth = %d;\n", Width);
	fprintf(File, "const int KImageHeight = %d;\n", Height);
	fprintf(File, "const unsigned char KImageData[%d * %d * 4] = {", Width, Height);

	for (int Index = 0; Index < Width * Height * 4; Index += 4) {
		if (Index % 32 == 0) {
			fprintf(File, "\n\t");
		}

		fprintf(
			File,
			"0x%02x, 0x%02x, 0x%02x, 0x%02x, ",
			Bytes[Index],
			Bytes[Index + 1],
			Bytes[Index + 2],
			Bytes[Index + 3]);

	}

	fprintf(File, "\n};\n");
	
	fprintf(File, "inline SDL_Surface *Data_CreateImageSurface()\n");
	fprintf(File, "{\n");
	fprintf(File, "\treturn SDL_CreateSurfaceFrom(KImageWidth, KImageHeight, SDL_PIXELFORMAT_BGRA32, (void*)KImageData, KImageWidth * 4);\n");
	fprintf(File, "}\n");

	fclose(File);
}
#endif
