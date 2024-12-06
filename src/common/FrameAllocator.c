#include "FrameAllocator.h"

#include <stb_ds.h>
#include <stdlib.h>

#include "Log.h"

struct Allocation {
	void* Key;
	bool Value;
};

struct {
	void* Memory;
	struct Allocation* TrackedAllocations;
	uint64 Head;
	uint64 Capacity;
	uint64 HighWaterMark;
} GFrameAlloc;

void FrameAllocatorInitialize(size_t Size)
{
	ZERO_STRUCT(&GFrameAlloc);
	GFrameAlloc.Capacity = Size;
	GFrameAlloc.Memory = malloc(GFrameAlloc.Capacity);

	hmdefault(GFrameAlloc.TrackedAllocations, false);

	ASSERT(GFrameAlloc.Memory);
}

void FrameAllocatorShutdown(void)
{
	free(GFrameAlloc.Memory);
	hmfree(GFrameAlloc.TrackedAllocations);
	ZERO_STRUCT(&GFrameAlloc);
}

void FrameAllocatorNextFrame(void)
{
	ASSERT(hmlen(GFrameAlloc.TrackedAllocations) == 0 && "All frame allocations must be freed with call to FrameFree before next frame.");
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

		hmput(GFrameAlloc.TrackedAllocations, Result, true);
	} else {
		PanicAndAbort("Frame Allocator Panic", "Frame allocator exceeded capacity!");
	}

	return Result;
}

void FrameFree(void* Pointer)
{
	bool IsTrackedAllocation = hmgeti(GFrameAlloc.TrackedAllocations, Pointer) >= 0;
	ASSERT(IsTrackedAllocation);
	hmdel(GFrameAlloc.TrackedAllocations, Pointer);
}