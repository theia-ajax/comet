#include <SDL3/SDL.h>
#include <math.h>
#include <sokol_time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/Game.h"

static SDL_Window* GWindow = NULL;

void HandleExit(int Status, void* Arg)
{
	GameShutdown();
	SDL_Quit();
}

int main(int argc, char* argv[])
{
	stm_setup();
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* Window = SDL_CreateWindow("Comet", 1920, 1080, SDL_WINDOW_RESIZABLE);
	GWindow = Window;
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

	// SDL_WINDOWPOS_CENTERED doesn't seem to include window decoration which is especially noticable on the Y axis
	// Manually smudging the window position to make it more centered for now.
	SDL_DisplayID WindowDisplayId = SDL_GetDisplayForWindow(Window);
	SDL_Rect DisplayBounds;
	SDL_GetDisplayBounds(WindowDisplayId, &DisplayBounds);
	int WindowHeight;
	SDL_GetWindowSize(Window, NULL, &WindowHeight);
	int WindowY = DisplayBounds.y + (DisplayBounds.h - (WindowHeight + 96.0f)) / 2;
	SDL_SetWindowPosition(Window, SDL_WINDOWPOS_CENTERED, WindowY);

	size_t MemoryBytes = 64 * 1024;
	void* Memory = malloc(MemoryBytes);

	bool Success = GameInitialize(&(GameInitParams){
		.Memory = Memory,
		.MemorySizeInBytes = MemoryBytes,
		.Window = Window,
	});
	
	on_exit(HandleExit, NULL);

	ASSERT(Success);

	const int KTargetFramesPerSecond = 0;
	double KTargetFrameRateSeconds = (KTargetFramesPerSecond != 0) ? (1.0 / KTargetFramesPerSecond) : 0.0;
	uint64 NowTicks = 0;
	uint64 DeltaTicks = 0;
	uint64 SimTimeTicks = 0;
	double ElapsedSeconds = 0.0;

	GameInput InputState = {0};

	while (GameIsRunning()) {
		uint64 FrameStartTicks = stm_now();
		// DebugNextFrame();

		SDL_Event Event;
		while (SDL_PollEvent(&Event)) {
			switch (Event.type) {
				case SDL_EVENT_QUIT: GameRequestShutdown(); break;
				case SDL_EVENT_KEY_DOWN:
					if (Event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) GameRequestShutdown();
					InputState.KeyStates[Event.key.keysym.scancode] = true;
					break;
				case SDL_EVENT_KEY_UP: InputState.KeyStates[Event.key.keysym.scancode] = false; break;
				default: break;
			}
			GameProcessEvent(&Event);
		}

		GameSendInput(&InputState);

		DeltaTicks = stm_laptime(&NowTicks);
		double DeltaTimeSeconds = stm_sec(DeltaTicks);
		ElapsedSeconds += DeltaTimeSeconds;

		GameTime Time = {
			.DeltaTime = DeltaTimeSeconds,
			.DeltaTimeF = (float)DeltaTimeSeconds,
			.ElapsedSeconds = ElapsedSeconds,
			.SimTimeMS = stm_ms(SimTimeTicks),
		};

		GameUpdate(&Time);
		SimTimeTicks = stm_since(FrameStartTicks);

		GameRender(&Time);

		while (KTargetFramesPerSecond != 0 && stm_sec(stm_since(FrameStartTicks)) < KTargetFrameRateSeconds) {
			// Do nothing...
		};

		// game->game_state.game_frame++;
	}

	// Should actually figure out if I'm going to do things this way or not...
	free(Memory);

	return 0;
}

[[_Noreturn]] void PanicAndAbort(const char* Title, const char* Message)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, Title, Message, GWindow);
	exit(1);
}