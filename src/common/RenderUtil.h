#pragma once

#include "Math2D.h"

typedef struct SDL_Renderer SDL_Renderer;

const char* SelectRenderDriver(const char* RequestedRenderDriver);
void SDL_RenderAABB(SDL_Renderer* Renderer, const AABB Box);
void SDL_RenderCircle(SDL_Renderer* Renderer, const Vec2 Center, const float32 Radius);