#include "Application.h"

#include <SDL3/SDL.h>
#include <sokol_time.h>

#include "Game.h"
#include "Log.h"

typedef struct _Application {
	SDL_Window* Window;
} _Application;

static const ApplicationConfig DefaultApplicationConfig = {};

Application* ApplicationInitialize(const ApplicationConfig* Config)
{
	Config = (Config != NULL) ? Config : &DefaultApplicationConfig;

	Application* Result = NULL;

	_Application* App = SDL_malloc(sizeof(_Application));

	LoggingInitialize(LogLevel_Info);

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		PanicAndAbort("SDL Error", SDL_GetError());
	}

	int SdlVersion = SDL_GetVersion();

	LogInfo(
		"System: Initialized SDL v%d.%d.%d compiled with v%d.%d.%d",
		SDL_VERSIONNUM_MAJOR(SdlVersion),
		SDL_VERSIONNUM_MINOR(SdlVersion),
		SDL_VERSIONNUM_MICRO(SdlVersion),
		SDL_VERSIONNUM_MAJOR(SDL_VERSION),
		SDL_VERSIONNUM_MINOR(SDL_VERSION),
		SDL_VERSIONNUM_MICRO(SDL_VERSION));

	stm_setup();

	SDL_Window* Window = SDL_CreateWindow("Comet", 1440, 320, SDL_WINDOW_RESIZABLE);
	App->Window = Window;

	// SDL_WINDOWPOS_CENTERED doesn't seem to include window decoration which is especially noticable on the Y axis
	// Manually smudging the window position to make it more centered for now.
	SDL_DisplayID WindowDisplayId = SDL_GetDisplayForWindow(Window);
	SDL_Rect DisplayBounds;
	SDL_GetDisplayBounds(WindowDisplayId, &DisplayBounds);
	int WindowHeight;
	SDL_GetWindowSize(Window, NULL, &WindowHeight);
	int WindowY = DisplayBounds.y + (DisplayBounds.h - (WindowHeight + 96.0f)) / 2;
	SDL_SetWindowPosition(Window, SDL_WINDOWPOS_CENTERED, WindowY);

	bool Success = GameInitialize(&(GameInitParams){
		.Window = Window,
	});

	ASSERT(Success);

	Result = (Application*)App;

	return Result;
}

void ApplicationShutdown(Application* App)
{
	GameShutdown();
	SDL_Quit();
}

void ApplicationRun(Application* App)
{
	_Application* _App = (_Application*)App;

	SDL_Window* Window = _App->Window;

	const int KTargetFramesPerSecond = 60;
	double KTargetFrameRateSeconds = (KTargetFramesPerSecond != 0) ? (1.0 / KTargetFramesPerSecond) : 0.0;
	uint64 NowTicks = 0;
	uint64 DeltaTicks = 0;
	uint64 SimTimeTicks = 0;
	uint64 RenderTimeTicks = 0;
	double ElapsedSeconds = 0.0;

	GameInput InputState = {0};

	while (GameIsRunning()) {
		uint64 FrameStartTicks = stm_now();

		SDL_Event Event;
		while (SDL_PollEvent(&Event)) {
			switch (Event.type) {
				case SDL_EVENT_QUIT: GameRequestShutdown(); break;
				case SDL_EVENT_KEY_DOWN:
					if (Event.key.scancode == SDL_SCANCODE_ESCAPE) {
						GameRequestShutdown();
					}
					if (Event.key.scancode == SDL_SCANCODE_RETURN && (Event.key.mod & SDL_KMOD_ALT) != 0) {
						SDL_SetWindowFullscreen(Window, !((SDL_GetWindowFlags(Window) & SDL_WINDOW_FULLSCREEN) != 0));
					}
					InputState.KeyStates[Event.key.scancode] = true;
					break;
				case SDL_EVENT_KEY_UP: InputState.KeyStates[Event.key.scancode] = false; break;
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
			.RenderTimeMS = stm_ms(RenderTimeTicks),
		};

		uint64 SimStartTicks = stm_now();
		GameUpdate(&Time);
		SimTimeTicks = stm_since(SimStartTicks);

		uint64 RenderStartTicks = stm_now();
		GameRender(&Time);
		RenderTimeTicks = stm_since(RenderStartTicks);

		while (KTargetFramesPerSecond != 0 && stm_sec(stm_since(FrameStartTicks)) < KTargetFrameRateSeconds) {
			// Do nothing...
		};
	}
}
