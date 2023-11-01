#include "Debug.h"

#include <SDL2/SDL.h>
#include <stdarg.h>

#define SYSFONT_U8 uint8
#define SYSFONT_U16 uint16
#define SYSFONT_U32 uint32
#define SYSFONT_IMPLEMENTATION
#include "sysfont.h"

#include "Game.h"

struct line {
	SDL_Point Points[16];
	int32 PointCount;
	int32 FramesRemaining;
	SDL_Color Color;
};

static struct {
	struct line LinesRingBuffer[1024 * 4];
	int32 LinesRingIndex;
	SDL_Surface* Canvas;
	SDL_Texture* CanvasTexture;
	SDL_Point CursorPos;
} GDebug;

static void DebugCreateOrFixupCanvas(SDL_Renderer* renderer);

void DebugInitialize(const DebugConfig* config)
{
	DebugConfig Config = (config != NULL) ? *config
										  : (DebugConfig){
											  .CanvasWidth = 320,
											  .CanvasHeight = 180,
										  };

	GDebug.Canvas = SDL_CreateRGBSurfaceWithFormat(
		0, Config.CanvasWidth, Config.CanvasHeight, 32, SDL_PIXELFORMAT_ARGB8888);
}

void DebugShutdown(void)
{
	SDL_FreeSurface(GDebug.Canvas);
	SDL_DestroyTexture(GDebug.CanvasTexture);
}

void DebugNextFrame(void)
{
	for (int32 Index = 0; Index < ARRAY_COUNT(GDebug.LinesRingBuffer); Index++) {
		if (GDebug.LinesRingBuffer[Index].FramesRemaining > 0) {
			GDebug.LinesRingBuffer[Index].FramesRemaining--;
		}
	}

	SDL_FillRect(
		GDebug.Canvas, NULL, 0x00000000);
	GDebug.CursorPos = (SDL_Point){0};
}

void DebugDraw(SDL_Renderer* renderer)
{
	for (int32 RawIndex = 0; RawIndex < ARRAY_COUNT(GDebug.LinesRingBuffer); RawIndex++) {
		int32 Index = (GDebug.LinesRingIndex + ARRAY_COUNT(GDebug.LinesRingBuffer))
					  % ARRAY_COUNT(GDebug.LinesRingBuffer);
		Index = RawIndex;
		if (GDebug.LinesRingBuffer[Index].FramesRemaining != 0) {
			SDL_Color color = GDebug.LinesRingBuffer[Index].Color;
			SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
			SDL_RenderDrawLines(
				renderer,
				GDebug.LinesRingBuffer[Index].Points,
				GDebug.LinesRingBuffer[Index].PointCount);
		}
	}

	if (GDebug.CanvasTexture == NULL) {
		GDebug.CanvasTexture = SDL_CreateTexture(
			renderer,
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING,
			GDebug.Canvas->w,
			GDebug.Canvas->h);
		SDL_SetTextureBlendMode(GDebug.CanvasTexture, SDL_BLENDMODE_BLEND);
	}

	SDL_Surface* CanvasTextureSurface;
	if (SDL_LockTextureToSurface(GDebug.CanvasTexture, NULL, &CanvasTextureSurface) == 0) {
		SDL_FillRect(CanvasTextureSurface, NULL, 0x00000000);
		SDL_BlitSurface(GDebug.Canvas, NULL, CanvasTextureSurface, NULL);
		SDL_UnlockTexture(GDebug.CanvasTexture);
	}

	SDL_RenderCopy(
		renderer,
		GDebug.CanvasTexture,
		NULL,
		&(SDL_Rect){.w = GDebug.Canvas->w, .h = GDebug.Canvas->h});
}

static int32 NextIndex()
{
	int32 Index = GDebug.LinesRingIndex;
	GDebug.LinesRingIndex++;
	if (GDebug.LinesRingIndex >= ARRAY_COUNT(GDebug.LinesRingBuffer)) {
		GDebug.LinesRingIndex = 0;
	}
	return Index;
}

void DebugLine(float x0, float y0, float x1, float y1, Color color, int frames)
{
#if 0
#define line(index) g_debug.lines_ring_buffer[index]
#define line_point(index, point) line(index).points[point]
	int32 index= NextIndex();
	line(index).FramesRemaining= frames;
	line(index).PointCount= 2;
	line(index).color= *((SDL_Color*)&color);
	world_fpoint_to_screen_point(x0, y0, &line_point(index, 0).x, &line_point(index, 0).y);
	world_fpoint_to_screen_point(x1, y1, &line_point(index, 1).x, &line_point(index, 1).y);
#undef line_point
#undef line
#endif
}

void DebugBox(float x0, float y0, float x1, float y1, Color color, int frames)
{
#if 0
#define line(index) g_debug.lines_ring_buffer[index]
#define line_point(index, point) line(index).points[point]
	int32 index= NextIndex();
	line(index).FramesRemaining= frames;
	line(index).PointCount= 5;
	line(index).color= *((SDL_Color*)&color);

	world_fpoint_to_screen_point(x0, y0, &line_point(index, 0).x, &line_point(index, 0).y);
	world_fpoint_to_screen_point(x1, y0, &line_point(index, 1).x, &line_point(index, 1).y);
	world_fpoint_to_screen_point(x1, y1, &line_point(index, 2).x, &line_point(index, 2).y);
	world_fpoint_to_screen_point(x0, y1, &line_point(index, 3).x, &line_point(index, 3).y);
	world_fpoint_to_screen_point(x0, y0, &line_point(index, 4).x, &line_point(index, 4).y);
#undef line_point
#undef line
#endif
}

void DebugSetCursorXY(int32 x, int32 y)
{
	GDebug.CursorPos.x = x;
	GDebug.CursorPos.y = y;
}

void DebugGetCursorXY(int32* x, int32* y)
{
	if (x) *x = GDebug.CursorPos.x;
	if (y) *y = GDebug.CursorPos.y;
}

void DebugPrintf(const char* format, ...)
{
	char Buffer[1024];
	va_list Args;
	va_start(Args, format);
	SDL_vsnprintf(Buffer, ARRAY_COUNT(Buffer), format, Args);
	va_end(Args);

	sysfont_8x8_u32(
		GDebug.Canvas->pixels,
		GDebug.Canvas->w,
		GDebug.Canvas->h,
		GDebug.CursorPos.x,
		GDebug.CursorPos.y,
		Buffer,
		0xFFFFFF00);

	GDebug.CursorPos.x = 0;
	GDebug.CursorPos.y += 8;
}
