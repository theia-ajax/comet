#pragma once

#include "Types.h"

DEFINE_HANDLE(VideoDecoderId);
typedef struct SDL_Surface SDL_Surface;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Renderer SDL_Renderer;

VideoDecoderId CreateVideoDecoderFromFile(const char *FileName);
void DestroyVideoDecoder(VideoDecoderId VideoDecoderHandle);
void VideoDecoderUpdate(VideoDecoderId VideoDecoderHandle, float32 DeltaTime);
SDL_Texture *VideoDecoderRenderNextFrame(VideoDecoderId VideoDecoderHandle, SDL_Renderer *Renderer);