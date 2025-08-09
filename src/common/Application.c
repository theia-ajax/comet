#include "Application.h"

#include <SDL3/SDL.h>
#include <sokol_time.h>

#include "Game.h"
#include "Log.h"

typedef struct _Application {
	SDL_Window* Window;
} _Application;

static const ApplicationConfig DefaultApplicationConfig = {};

void VidTest(void);

Application* ApplicationInitialize(const ApplicationConfig* Config)
{
	Config = (Config != NULL) ? Config : &DefaultApplicationConfig;

	Application* Result = NULL;

	_Application* App = SDL_malloc(sizeof(_Application));

	LoggingInitialize(LogLevel_Info);

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

	SDL_Window* Window = SDL_CreateWindow("Comet", 1480, 320, SDL_WINDOW_RESIZABLE);
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

	VidTest();

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

SDL_Window* GetApplicationWindow(Application* App)
{
	return ((_Application*)App)->Window;
}

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

void SaveFrame(int32 FrameNum, void* Data, int32 Width, int32 Height, int32 Comp);

void VidTest(void)
{
	AVFormatContext* FormatContext = NULL;
	if (avformat_open_input(&FormatContext, "assets/aos.mp4", NULL, NULL) != 0) {
		LogError("Couldn't open video file");
		return;
	}

	if (avformat_find_stream_info(FormatContext, NULL) < 0) {
		LogError("Couldn't find stream information");
		goto close_input_and_exit;
	}

	av_dump_format(FormatContext, 0, "assets/aos.mp4", 0);

	int VideoStream = -1;
	for (int StreamIndex = 0; StreamIndex < FormatContext->nb_streams; StreamIndex++) {
		if (FormatContext->streams[StreamIndex]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			VideoStream = StreamIndex;
		}
	}

	if (VideoStream < 0) {
		LogError("Couldn't find video stream");
		goto close_input_and_exit;
	} else {
		LogInfo("Video stream index: %d", VideoStream);
	}

	AVCodecParameters* CodecParameters = FormatContext->streams[VideoStream]->codecpar;
	const AVCodec* Codec = avcodec_find_decoder(CodecParameters->codec_id);

	if (Codec == NULL) {
		LogError("Unsupported codec");
		goto close_input_and_exit;
	}

	AVCodecContext* CodecContext = avcodec_alloc_context3(Codec);
	avcodec_parameters_to_context(CodecContext, CodecParameters);

	if (avcodec_open2(CodecContext, Codec, NULL) < 0) {
		LogError("Could not open codec");
		goto close_input_and_exit;
	}

	AVFrame* Frame = NULL;
	Frame = av_frame_alloc();
	AVFrame* FrameRGB = av_frame_alloc();

	int FrameSizeBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, CodecContext->width, CodecContext->height, 32);
	uint8* FrameBuffer = (uint8*)av_malloc(FrameSizeBytes);

	if (av_image_fill_arrays(
			FrameRGB->data,
			FrameRGB->linesize,
			FrameBuffer,
			AV_PIX_FMT_RGB24,
			CodecContext->width,
			CodecContext->height,
			32) < 0)
	{
		LogError("Failed");
	}

	struct SwsContext* SwsContext = sws_getContext(
		CodecContext->width,
		CodecContext->height,
		CodecContext->pix_fmt,
		CodecContext->width,
		CodecContext->height,
		AV_PIX_FMT_RGB24,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL);
	AVPacket* Packet = av_packet_alloc();

	int32 FrameIndex = 0;
	while (av_read_frame(FormatContext, Packet) >= 0) {
		if (Packet->stream_index == VideoStream) {
			int Ret = avcodec_send_packet(CodecContext, Packet);

			while (Ret >= 0) {
				Ret = avcodec_receive_frame(CodecContext, Frame);

				sws_scale(
					SwsContext,
					(uint8 const* const*)Frame->data,
					Frame->linesize,
					0,
					CodecContext->height,
					FrameRGB->data,
					FrameRGB->linesize);

				if (FrameIndex == 1) {
					SaveFrame(FrameIndex, FrameRGB->data, CodecContext->width, CodecContext->height, 3);
				}
			}
		}
		av_packet_unref(Packet);
		FrameIndex++;
	}

	// av_free(FrameBuffer);

close_input_and_exit:
	avformat_close_input(&FormatContext);
	return;
}

#include "stb_image_write.h"
void SaveFrame(int32 FrameNum, void* Data, int32 Width, int32 Height, int32 Comp)
{
	char FileName[64];
	snprintf(FileName, SDL_arraysize(FileName), "Frame_%08d.png");
	stbi_write_png(FileName, Width, Height, Comp, Data, Comp * 8 * Width);
}