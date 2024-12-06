#pragma once

#include "Types.h"

void FrameAllocatorInitialize(size_t Size);
void FrameAllocatorShutdown(void);
void FrameAllocatorNextFrame(void);

void* FrameAlloc(size_t Size);
void FrameFree(void* Pointer);