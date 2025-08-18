#include "VideoDecoder.h"

#include <SDL3/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stb_ds.h>

#include "Debug.h"
#include "Game.h"
#include "Log.h"

enum { KMaxVideoDecoders = 4 };

typedef struct VideoDecoder {
	AVFormatContext* FormatContext;
	int VideoStreamIndex;
	AVCodecContext* CodecContext;
	AVFrame* Frame;
	AVFrame* FrameRgb;
	uint8* FrameBuffer;
	struct SwsContext* SwsContext;
	AVPacket* Packet;
	SDL_Texture* Texture;
	float32 ElapsedSeconds;
	bool Started;
} VideoDecoder;

VideoDecoder Storage[KMaxVideoDecoders];
handint* StorageFreeStack;
handint NextId;

VideoDecoderId CreateVideoDecoderFromFile(const char* FileName)
{
	VideoDecoderId Result = (VideoDecoderId){NONE};

	handint Id = NONE;
	if (arrlen(StorageFreeStack) > 0) {
		Id = arrpop(StorageFreeStack);
	}
	if (NextId < KMaxVideoDecoders) {
		Id = NextId++;
	}

	if (Id == NONE) {
		LogError("No remaining video decoder storage.");
	} else {

		VideoDecoder* Decoder = &Storage[Id];
		SDL_zerop(Decoder);

		Decoder->FormatContext = NULL;

		if (avformat_open_input(&Decoder->FormatContext, FileName, NULL, NULL) != 0) {
			LogError("Couldn't open video file");
		} else {
			if (avformat_find_stream_info(Decoder->FormatContext, NULL) < 0) {
				LogError("Couldn't find stream information");
			} else {
				Decoder->VideoStreamIndex = -1;
				for (int StreamIndex = 0; StreamIndex < Decoder->FormatContext->nb_streams; StreamIndex++) {
					if (Decoder->FormatContext->streams[StreamIndex]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
						Decoder->VideoStreamIndex = StreamIndex;
					}
				}
				if (Decoder->VideoStreamIndex < 0) {
					LogError("Couldn't find video stream");
				} else {
					LogInfo("Video stream index: %d", Decoder->VideoStreamIndex);
					AVCodecParameters* CodecParameters =
						Decoder->FormatContext->streams[Decoder->VideoStreamIndex]->codecpar;
					const AVCodec* Codec = avcodec_find_decoder(CodecParameters->codec_id);

					if (Codec == NULL) {
						LogError("Unsupported codec");
					} else {
						Decoder->CodecContext = avcodec_alloc_context3(Codec);
						avcodec_parameters_to_context(Decoder->CodecContext, CodecParameters);

						if (avcodec_open2(Decoder->CodecContext, Codec, NULL) < 0) {
							LogError("Could not open codec");
						} else {
							Decoder->Frame = av_frame_alloc();
							Decoder->FrameRgb = av_frame_alloc();

							enum AVPixelFormat Format = AV_PIX_FMT_BGR32;
							int FrameSizeBytes = av_image_get_buffer_size(
								Format,
								Decoder->CodecContext->width,
								Decoder->CodecContext->height,
								32);
							Decoder->FrameBuffer = (uint8*)av_malloc(FrameSizeBytes);
							if (av_image_fill_arrays(
									Decoder->FrameRgb->data,
									Decoder->FrameRgb->linesize,
									Decoder->FrameBuffer,
									Format,
									Decoder->CodecContext->width,
									Decoder->CodecContext->height,
									32) < 0)
							{
								LogError("Failed");
							} else {
								Decoder->SwsContext = sws_getContext(
									Decoder->CodecContext->width,
									Decoder->CodecContext->height,
									Decoder->CodecContext->pix_fmt,
									Decoder->CodecContext->width,
									Decoder->CodecContext->height,
									Format,
									SWS_BILINEAR,
									NULL,
									NULL,
									NULL);
								Decoder->Packet = av_packet_alloc();

								Result.Value = Id;
							}
						}
					}
				}
			}
		}
	}

	return Result;
}

void DestroyVideoDecoder(VideoDecoderId VideoDecoderHandle)
{
	ASSERT(false && "TODO");
}

void VideoDecoderSeekSeconds(VideoDecoderId VideoDecoderHandle, float32 Seconds)
{
	// if (!VALID_INDEX(VideoDecoderHandle.Value, KMaxVideoDecoders)) {
	// 	return;
	// }

	// VideoDecoder* Decoder = &Storage[VideoDecoderHandle.Value];

	// int SeekFlags = AVSEEK_FLAG_ANY;
	
	// const int StreamIndex = Decoder->VideoStreamIndex;
	// const int TimeBaseNumerator = Decoder->FormatContext->streams[StreamIndex]->time_base.num;
	// const int TimeBaseDenominator = Decoder->FormatContext->streams[StreamIndex]->time_base.den;
	// const float64 TimeBase = (float64)TimeBaseNumerator / (float64)TimeBaseDenominator;
	// const int64_t DurationTimestamp = Decoder->FormatContext->streams[StreamIndex]->duration;
	// const float32 DurationSeconds = DurationTimestamp * TimeBase;
	
	// if (!Decoder->Started) {
	// 	Decoder->Started = true;
	// 	SeekFlags = AVSEEK_FLAG_FRAME;
	// } else {
	// 	Decoder->ElapsedSeconds += DeltaTime;
	// 	if (Decoder->ElapsedSeconds > DurationSeconds) {
	// 		Decoder->ElapsedSeconds -= DurationSeconds;
	// 		SeekFlags = AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD;
	// 	}
	// }
	
	// const int64 ElapsedTimestamp = (int64)(Decoder->ElapsedSeconds / TimeBase);
	// av_seek_frame(Decoder->FormatContext, StreamIndex, ElapsedTimestamp, SeekFlags);
}

float32 VideoDecoderGetDurationSeconds(VideoDecoderId VideoDecoderHandle)
{

}

void VideoDecoderUpdate(VideoDecoderId VideoDecoderHandle, float32 DeltaTime)
{
	if (!VALID_INDEX(VideoDecoderHandle.Value, KMaxVideoDecoders)) {
		return;
	}

	VideoDecoder* Decoder = &Storage[VideoDecoderHandle.Value];

	int SeekFlags = AVSEEK_FLAG_ANY;
	
	const int StreamIndex = Decoder->VideoStreamIndex;
	const int TimeBaseNumerator = Decoder->FormatContext->streams[StreamIndex]->time_base.num;
	const int TimeBaseDenominator = Decoder->FormatContext->streams[StreamIndex]->time_base.den;
	const float64 TimeBase = (float64)TimeBaseNumerator / (float64)TimeBaseDenominator;
	const int64_t DurationTimestamp = Decoder->FormatContext->streams[StreamIndex]->duration;
	const float32 DurationSeconds = DurationTimestamp * TimeBase;
	
	if (!Decoder->Started) {
		Decoder->Started = true;
		SeekFlags = AVSEEK_FLAG_FRAME;
	} else {
		Decoder->ElapsedSeconds += DeltaTime;
		if (Decoder->ElapsedSeconds > DurationSeconds) {
			Decoder->ElapsedSeconds -= DurationSeconds;
			SeekFlags = AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD;
		}
	}
	
	const int64 ElapsedTimestamp = (int64)(Decoder->ElapsedSeconds / TimeBase);
	av_seek_frame(Decoder->FormatContext, StreamIndex, ElapsedTimestamp, SeekFlags);

	DebugPrintf(
		"SEC: %0.3f/%0.3f   FRM: %llu/%llu",
		Decoder->ElapsedSeconds,
		DurationSeconds,
		ElapsedTimestamp,
		DurationTimestamp);
}

SDL_Texture* VideoDecoderRenderNextFrame(
	VideoDecoderId VideoDecoderHandle,
	SDL_Renderer* Renderer,
	const GameTime* gameTime)
{
	SDL_Texture* Result = NULL;

	if (!VALID_INDEX(VideoDecoderHandle.Value, KMaxVideoDecoders)) {
		return Result;
	}

	VideoDecoder* Decoder = &Storage[VideoDecoderHandle.Value];

	// if (av_read_frame(Decoder->FormatContext, Decoder->Packet) >= 0)

	int ReadFrameRet = av_read_frame(Decoder->FormatContext, Decoder->Packet);
	// if (ReadFrameRet < 0){
	// 	av_seek_frame(Decoder->FormatContext, Decoder->VideoStreamIndex, 0, AVSEEK_FLAG_ANY);
	// 	ReadFrameRet = av_read_frame(Decoder->FormatContext, Decoder->Packet);
	// }

	if (ReadFrameRet >= 0) {
		if (Decoder->Packet->stream_index == Decoder->VideoStreamIndex) {
			bool GotFrame = false;
			int SafetyValve = 12;

			do {
				int Ret = avcodec_send_packet(Decoder->CodecContext, Decoder->Packet);

				while (Ret >= 0) {
					Ret = avcodec_receive_frame(Decoder->CodecContext, Decoder->Frame);

					if (Ret >= 0) {
						sws_scale(
							Decoder->SwsContext,
							(uint8 const* const*)Decoder->Frame->data,
							Decoder->Frame->linesize,
							0,
							Decoder->CodecContext->height,
							Decoder->FrameRgb->data,
							Decoder->FrameRgb->linesize);
						GotFrame = true;
					}
				}
			} while (!GotFrame && --SafetyValve > 0);

			LogInfo("%d", SafetyValve);
		}
	}

	if (Decoder->Texture == NULL) {
		Decoder->Texture = SDL_CreateTexture(
			Renderer,
			SDL_PIXELFORMAT_RGBA32,
			SDL_TEXTUREACCESS_STREAMING,
			Decoder->CodecContext->width,
			Decoder->CodecContext->height);
	}

	SDL_Surface* Surface = NULL;
	if (SDL_LockTextureToSurface(Decoder->Texture, NULL, &Surface)) {
		SDL_memcpy(
			Surface->pixels,
			Decoder->FrameRgb->data[0],
			Decoder->FrameRgb->linesize[0] * Decoder->CodecContext->height);

		SDL_UnlockTexture(Decoder->Texture);
	}

	return Decoder->Texture;
}