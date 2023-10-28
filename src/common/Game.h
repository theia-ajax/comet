#pragma once

#include "types.h"

typedef union SDL_Event SDL_Event;
typedef struct SDL_Window SDL_Window;

typedef struct GameInitParams {
	void* Memory;
	size_t MemorySizeInBytes;
	SDL_Window* Window;
} GameInitParams;

typedef struct GameInput {
	bool KeyStates[512];
} GameInput;

typedef struct GameTime {
	flt64 ElapsedSeconds;
	flt64 DeltaTime;
	flt64 SimTimeMS;
	flt32 DeltaTimeF;
} GameTime;

bool GameInitialize(const GameInitParams* params);
void GameShutdown(void);
void GameSendInput(const GameInput* input);
void GameProcessEvent(const SDL_Event* event);
void GameUpdate(const GameTime* time);
void GameRender(const GameTime* time);
bool GameIsRunning(void);
void GameRequestShutdown(void);
