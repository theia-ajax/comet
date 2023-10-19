#pragma once

#include "types.h"

typedef struct SDL_Renderer SDL_Renderer;

typedef struct Color { uint8 r, g, b, a; } Color;

void debug_next_frame();
void debug_draw(SDL_Renderer* renderer);
void debug_line(float x0, float y0, float x1, float y1, Color color, int frames);
void debug_box(float x0, float y0, float x1, float y1, Color color, int frames);

