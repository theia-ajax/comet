#include "Application.h"

#include <SDL3/SDL.h>
#include <sokol_time.h>

#include "Game.h"
#include "Log.h"
#include "SdlEventHandler.h"

typedef struct _Application {
	SDL_Window* Window;
	GameInput InputState;
} _Application;

static const ApplicationConfig DefaultApplicationConfig = {};

bool ApplicationHandleSdlEvent(const SDL_Event* Event, void* Context);

Application* ApplicationInitialize(const ApplicationConfig* Config)
{
#ifdef _DEBUG
	LogLevel LoggingLevel = LogLevel_Info;
#else
	LogLevel LoggingLevel = LogLevel_Warning;
#endif

	LoggingLevel = LogLevel_Info;
	LoggingInitialize(LoggingLevel);
	LogInfo(__FUNCTION__);

	Config = (Config != NULL) ? Config : &DefaultApplicationConfig;

	Application* Result = NULL;
	_Application* App = SDL_malloc(sizeof(_Application));
	SDL_zerop(App);

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		PanicAndAbort("SDL Error", SDL_GetError());
	}
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");

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

	AddSdlEventHandler(ApplicationHandleSdlEvent, App);

	SDL_Window* Window = SDL_CreateWindow("Comet", 1920, 1080, SDL_WINDOW_RESIZABLE);
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

	// VidTest();

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

	while (GameIsRunning()) {
		uint64 FrameStartTicks = stm_now();

		SDL_Event Event;
		while (SDL_PollEvent(&Event)) {
			HandleSdlEvent(&Event);
		}

		GameSendInput(&_App->InputState);

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
			const double TilNextFrameMS = stm_ms(stm_since(FrameStartTicks));
			if (TilNextFrameMS > 1000) {
				// SDL_DelayNS(TilNextFrameMS);
			}
			// Do nothing...
		};
	}
}

SDL_Window* GetApplicationWindow(Application* App)
{
	return ((_Application*)App)->Window;
}

bool ApplicationHandleSdlEvent(const SDL_Event* Event, void* Context)
{
	_Application* App = (_Application*)Context;
	bool Handled = false;

	switch (Event->type) {
		case SDL_EVENT_QUIT:
			GameRequestShutdown();
			Handled = true;
			break;
		case SDL_EVENT_KEY_DOWN:
			switch (Event->key.scancode) {
				case SDL_SCANCODE_ESCAPE:
					GameRequestShutdown();
					Handled = true;
					break;
				case SDL_SCANCODE_RETURN:
					if ((Event->key.mod & SDL_KMOD_ALT) != 0) {
						SDL_SetWindowFullscreen(
							App->Window,
							!((SDL_GetWindowFlags(App->Window) & SDL_WINDOW_FULLSCREEN) != 0));
						Handled = true;
					}
					break;
			}
			App->InputState.KeyStates[Event->key.scancode] = true;
			break;
		case SDL_EVENT_KEY_UP: App->InputState.KeyStates[Event->key.scancode] = false; break;
		default: break;
	}

	return Handled;
}