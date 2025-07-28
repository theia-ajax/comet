#pragma once

#include "Types.h"

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_FRect SDL_FRect;

typedef struct Color {
	uint8 r, g, b, a;
} Color;

typedef struct DebugConfig {
	int32 CanvasWidth, CanvasHeight;
	SDL_Renderer* Renderer;
	uint32 BackgroundColor;
	uint32 ForegroundColor;
	uint32 Margin;
} DebugConfig;

void DebugInitialize(const DebugConfig* config);
void DebugShutdown(void);

void DebugNextFrame(void);
void DebugDraw(SDL_Renderer* renderer);

void DebugLine(float x0, float y0, float x1, float y1, Color color, int frames);
void DebugBox(float x0, float y0, float x1, float y1, Color color, int frames);

void DebugSetCursorXY(int32 x, int32 y);
void DebugGetCursorXY(int32* x, int32* y);
void DebugPrintf(const char* format, ...);