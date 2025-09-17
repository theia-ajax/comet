#include "TGui.h"

#include "Log.h"
#include "Math2D.h"
#include "StringId.h"
#include <SDL3/SDL.h>
#include <stb_ds.h>

// forward declarations
typedef struct TGuiWindow TGuiWindow;
typedef struct TGuiWindowMap TGuiWindowMap;

// private definitions
typedef struct TGuiContext {
	TGuiWindowMap* Windows;
	TGuiWindow* ActiveWindow;
} TGuiContext;

typedef struct TGuiWindow {
	Vec2 Position;
	Vec2 Size;
	StringId TitleId;
	Vec2 Cursor;
} TGuiWindow;

typedef struct TGuiWindowMap {
	StringId Key;
	TGuiWindow Value;
} TGuiWindowMap;

// static data

// private interface
void _TGuiResetActiveWindow(TGuiContext* Ctx);

// public implementations

TGuiContext* CreateTGui(const TGuiConfig* Config)
{
	TGuiContext* Ctx = (TGuiContext*)SDL_malloc(sizeof(TGuiContext));
	SDL_zerop(Ctx);

	_TGuiResetActiveWindow(Ctx);

	hmdefaults(
		Ctx->Windows,
		((TGuiWindowMap){
			.Key = KInvalidStringId,
			.Value =
				(TGuiWindow){
					.TitleId = KInvalidStringId,
				},
		}));

	return Ctx;
}

void DestroyTGui(TGuiContext* Ctx)
{
	if (Ctx != NULL) {
		hmfree(Ctx->Windows);
		SDL_free(Ctx);
	}
}

bool TGuiBeginWindow(TGuiContext* Ctx, const char* Title, int Width, int Height)
{
	if (Ctx->ActiveWindow != NULL) {
		LogError(
			"Mismatched TGui(Begin/End)Window calls. TGuiEndWindow must be called before a new window can be created with TGuiBeginWindow.");
		return false;
	}

	StringId TitleId = GetStringId(Title);

	if (hmgeti(Ctx->Windows, TitleId) < 0) {
		hmput(
			Ctx->Windows,
			TitleId,
			((TGuiWindow){
				.Position = V2(0, 0),
				.Size = V2(Width, Height),
				.TitleId = GetStringId(Title),
			}));
	}
	TGuiWindow* Window = hmgetp_null(Ctx->Windows, TitleId);
	Ctx->ActiveWindow = Window;
}

void TGuiEndWindow(TGuiContext* Ctx)
{
	if (Ctx->ActiveWindow == NULL) {
		LogError("Mismatched TGui(Begin/End)Window calls. TGuiBeginWindow must be called before TGuiEndWindow.");
		return;
	}
	_TGuiResetActiveWindow(Ctx);
}

void TGuiLabel(TGuiContext* Ctx, const char* String, int X, int Y)
{
}

bool TGuiButton(TGuiContext* Ctx, const char* Label, int X, int Y, int Width, int Height)
{
}

void TGuiNextFrame(TGuiContext* Ctx)
{
	ASSERT(Ctx->ActiveWindow == NULL);

	for (int32 Index = 0, Count = hmlen(Ctx->Windows); Index < Count; Index++) {
		Ctx->Windows[Index].Value.Cursor = V2(2, 2);
	}
}

bool TGuiProcessEvent(TGuiContext* Ctx, const SDL_Event* Event)
{
}

void TGuiRender(TGuiContext* Ctx, SDL_Renderer* Renderer)
{
	for (int32 Index = 0, Count = hmlen(Ctx->Windows); Index < Count; Index++) {
		TGuiWindow* Window = &Ctx->Windows[Index];

		SDL_FRect WindowRect = (SDL_FRect){
			.x = Window->Position.X,
			.y = Window->Position.Y,
			.w = Window->Size.X,
			.h = Window->Size.Y,
		};

		SDL_SetRenderDrawColor(Renderer, 0, 0, 255, 255);
		SDL_RenderFillRect(Renderer, &WindowRect);
		SDL_SetRenderDrawColor(Renderer, 255, 255, 255, 255);
		SDL_RenderRect(Renderer, &WindowRect);
	}
}

// private implementations

void _TGuiResetActiveWindow(TGuiContext* Ctx)
{
	Ctx->ActiveWindow = NULL;
}