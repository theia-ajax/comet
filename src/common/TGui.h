#pragma once

#include "Types.h"

typedef struct SDL_Renderer SDL_Renderer;
typedef union SDL_Event SDL_Event;

typedef struct TGuiContext TGuiContext;

typedef struct TGuiConfig {
	int32 Width, Height;
} TGuiConfig;


TGuiContext* CreateTGui(const TGuiConfig *Config);
void DestroyTGui(TGuiContext* Ctx);

bool TGuiBeginWindow(TGuiContext* Ctx, const char* Title, float32 Width, float32 Height);
void TGuiEndWindow(TGuiContext* Ctx);
bool TGuiButton(TGuiContext* Ctx, const char* Label, float32 X, float32 Y, float32 Width, float32 Height);

void TGuiNextFrame(TGuiContext* Ctx);
bool TGuiProcessEvent(TGuiContext* Ctx, const SDL_Event *Event);
void TGuiRender(TGuiContext* Ctx, SDL_Renderer* Renderer);