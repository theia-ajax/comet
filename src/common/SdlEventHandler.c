#include "SdlEventHandler.h"

#include <stb_ds.h>

struct {
	SdlEventHandler* Handlers;
} GEventHandler;

bool HandleSdlEvent(const SDL_Event* Event)
{
	for (int HandlerCount = arrlen(GEventHandler.Handlers), HandlerIndex = HandlerCount - 1; HandlerIndex >= 0;
		 HandlerIndex--)
	{
		const SdlEventHandler* Handler = &GEventHandler.Handlers[HandlerIndex];
		if (Handler->Handler(Event, Handler->Context)) {
			return true;
		}
	}

	return false;
}

void AddSdlEventHandler(SdlEventHandlerFunc Func, void* Context)
{
	arrput(
		GEventHandler.Handlers,
		((SdlEventHandler){
			.Handler = Func,
			.Context = Context,
		}));
}

void ClearSdlEventHandlers(void)
{
	for (int Index = 0, Count = arrlen(GEventHandler.Handlers); Index < Count; Index++) {
		SDL_free(GEventHandler.Handlers[Index].Context);
	}
	arrfree(GEventHandler.Handlers);
	GEventHandler.Handlers = NULL;
}