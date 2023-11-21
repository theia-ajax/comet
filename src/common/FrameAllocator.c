#include "FrameAllocator.h"

#include <stdlib.h>

#include "Log.h"

struct {
	void* Memory;
	uint64 Head;
	uint64 Capacity;
	uint64 HighWaterMark;
} GFrameAlloc;

void FrameAllocatorInitialize(size_t Size)
{
	ZERO_STRUCT(&GFrameAlloc);
	GFrameAlloc.Capacity = Size;
	GFrameAlloc.Memory = malloc(GFrameAlloc.Capacity);

	ASSERT(GFrameAlloc.Memory);
}

void FrameAllocatorShutdown(void)
{
	free(GFrameAlloc.Memory);
	ZERO_STRUCT(&GFrameAlloc);
}

void FrameAllocatorNextFrame(void)
{
	ASSERT(GFrameAlloc.Memory);
	if (GFrameAlloc.Head > GFrameAlloc.HighWaterMark)
	{
		GFrameAlloc.HighWaterMark = GFrameAlloc.Head;
		LogWarning("FrameAllocator: New high water mark of %llu bytes", GFrameAlloc.HighWaterMark);
	}
	GFrameAlloc.Head = 0;
}

void* FrameAlloc(size_t Size)
{
	ASSERT(GFrameAlloc.Head + Size <= GFrameAlloc.Capacity);

	void* Result = NULL;

	if (GFrameAlloc.Head + Size <= GFrameAlloc.Capacity) {
		Result = (uint8*)GFrameAlloc.Memory + GFrameAlloc.Head;
		GFrameAlloc.Head += Size;
	} else {
		PanicAndAbort("Frame Allocator Panic", "Frame allocator exceeded capacity!");
	}

	return Result;
}