#pragma once

#include "Types.h"

typedef union SDL_Event SDL_Event;

typedef bool (*SdlEventHandlerFunc)(const SDL_Event* Event, void* Context);

typedef struct SdlEventHandler {
	SdlEventHandlerFunc Handler;
	void* Context;
} SdlEventHandler;

bool HandleSdlEvent(const SDL_Event *Event);
void ClearSdlEventHandlers(void);
void AddSdlEventHandler(SdlEventHandlerFunc Func, void* Context);
