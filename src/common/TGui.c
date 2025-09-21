#include "TGui.h"

#include "Log.h"
#include "Math2D.h"
#include "StringId.h"
#include <SDL3/SDL.h>
#include <stb_ds.h>

#define TGUI_WIDGET_CAST(TGuiWidgetType, Super) (TGuiWidgetType*)(Super->SubClass);

// forward declarations
typedef struct TGuiContext TGuiContext;
typedef struct TGuiWindow TGuiWindow;
typedef struct TGuiWidget TGuiWidget;

// private definitions
typedef void* (*TGuiCreateWidgetCallback)(TGuiContext* Ctx, TGuiWidget* Super);
typedef void (*TGuiDestroyWidgetCallback)(TGuiContext* Ctx, TGuiWidget* Super);
typedef void (*TGuiNextFrameCallback)(TGuiContext* Ctx, TGuiWidget* Super);
typedef bool (*TGuiProcessEventCallback)(TGuiContext* Ctx, TGuiWidget* Super, const SDL_Event* Event);
typedef void (*TGuiRenderCallback)(TGuiContext* Ctx, TGuiWidget* Super, SDL_Renderer* Renderer);

typedef struct TGuiContext {
	TGuiWindow* WindowMap;
	TGuiWindow* ActiveWindow;
	TGuiWindow* FocusedWindow;
} TGuiContext;

typedef struct TGuiWindow {
	StringId Key;
	Vec2 Position;
	Vec2 Size;
	Vec2 Cursor;
	TGuiWidget* WidgetMap;
} TGuiWindow;

typedef struct TGuiWidget {
	StringId Key;
	TGuiWindow* OwningWindow;
	void* SubClass;
	Vec2 Position;
	Vec2 Size;
	TGuiCreateWidgetCallback OnCreate;
	TGuiDestroyWidgetCallback OnDestroy;
	TGuiNextFrameCallback OnNextFrame;
	TGuiProcessEventCallback OnProcessEvent;
	TGuiRenderCallback OnRender;
} TGuiWidget;

typedef struct TGuiButtonWidget {
	bool IsDown;
	bool WasDown;
} TGuiButtonWidget;

// static data

// private interface
void _TGuiResetActiveWindow(TGuiContext* Ctx);

//   widgets
void TGuiWidgetCreateSubclass(TGuiContext* Ctx, TGuiWidget* Self);

void* TGuiButtonCreate(TGuiContext* Ctx, TGuiWidget* Super);
void TGuiButtonDestroy(TGuiContext* Ctx, TGuiWidget* Super);
void TGuiButtonNextFrame(TGuiContext* Ctx, TGuiWidget* Super);
bool TGuiButtonProcessEvent(TGuiContext* Ctx, TGuiWidget* Super, const SDL_Event* Event);
void TGuiButtonRender(TGuiContext* Ctx, TGuiWidget* Super, SDL_Renderer* Renderer);

// public implementations

TGuiContext* CreateTGui(const TGuiConfig* Config)
{
	TGuiContext* Ctx = (TGuiContext*)SDL_malloc(sizeof(TGuiContext));
	SDL_zerop(Ctx);

	_TGuiResetActiveWindow(Ctx);

	hmdefaults(
		Ctx->WindowMap,
		((TGuiWindow){
			.Key = KInvalidStringId,
		}));

	return Ctx;
}

void DestroyTGui(TGuiContext* Ctx)
{
	if (Ctx != NULL) {
		hmfree(Ctx->WindowMap);
		SDL_free(Ctx);
	}
}

bool TGuiBeginWindow(TGuiContext* Ctx, const char* Title, float32 Width, float32 Height)
{
	if (Ctx->ActiveWindow != NULL) {
		LogError(
			"Mismatched TGui(Begin/End)Window calls. TGuiEndWindow must be called before a new window can be created with TGuiBeginWindow.");
		return false;
	}

	StringId TitleId = GetStringId(Title);

	if (hmgeti(Ctx->WindowMap, TitleId) < 0) {
		hmputs(
			Ctx->WindowMap,
			((TGuiWindow){
				.Key = TitleId,
				.Size = V2(Width, Height),
			}));
	}
	TGuiWindow* Window = hmgetp_null(Ctx->WindowMap, TitleId);
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

bool TGuiButton(TGuiContext* Ctx, const char* Label, float32 X, float32 Y, float32 Width, float32 Height)
{
	if (Ctx->ActiveWindow == NULL) {
		LogError("No active window for button '%s'", Label);
		return false;
	}

	StringId Key = GetStringId(Label);
	if (hmgeti(Ctx->ActiveWindow->WidgetMap, Key) < 0) {
		hmputs(
			Ctx->ActiveWindow->WidgetMap,
			((TGuiWidget){
				.Key = Key,
				.OwningWindow = Ctx->ActiveWindow,
				.OnCreate = TGuiButtonCreate,
				.OnDestroy = TGuiButtonDestroy,
				.OnNextFrame = TGuiButtonNextFrame,
				.OnProcessEvent = TGuiButtonProcessEvent,
				.OnRender = TGuiButtonRender,
			}));
		TGuiWidgetCreateSubclass(Ctx, hmgetp(Ctx->ActiveWindow->WidgetMap, Key));
	}
	TGuiWidget* Super = hmgetp(Ctx->ActiveWindow->WidgetMap, Key);

	Super->Position = V2(X, Y);
	Super->Size = V2(Width, Height);

	TGuiButtonWidget* Self = TGUI_WIDGET_CAST(TGuiButtonWidget, Super);
	return Self->WasDown && !Self->IsDown;
}

void TGuiNextFrame(TGuiContext* Ctx)
{
	ASSERT(Ctx->ActiveWindow == NULL);

	for (int32 WindowIndex = 0, WindowCount = hmlen(Ctx->WindowMap); WindowIndex < WindowCount; WindowIndex++) {
		TGuiWindow* Window = &Ctx->WindowMap[WindowIndex];
		Window->Cursor = V2(2, 2);
		for (int32 WidgetIndex = 0, WidgetCount = hmlen(Window->WidgetMap); WidgetIndex < WidgetCount; WidgetIndex++) {
			TGuiWidget* Widget = &Window->WidgetMap[WidgetIndex];
			Widget->OnNextFrame(Ctx, Widget);
		}
	}
}

bool TGuiProcessEvent(TGuiContext* Ctx, const SDL_Event* Event)
{
	for (int32 WindowIndex = 0, WindowCount = hmlen(Ctx->WindowMap); WindowIndex < WindowCount; WindowIndex++) {
		TGuiWindow* Window = &Ctx->WindowMap[WindowIndex];
		for (int32 WidgetIndex = 0, WidgetCount = hmlen(Window->WidgetMap); WidgetIndex < WidgetCount; WidgetIndex++) {
			TGuiWidget* Widget = &Window->WidgetMap[WidgetIndex];

			// TODO check event is within window or some kind of focused window paradigm or something
			if (Widget->OnProcessEvent(Ctx, Widget, Event)) {
				return true;
			}
		}
	}

	return false;
}

void TGuiRender(TGuiContext* Ctx, SDL_Renderer* Renderer)
{
	for (int32 WindowIndex = 0, WindowCount = hmlen(Ctx->WindowMap); WindowIndex < WindowCount; WindowIndex++) {
		TGuiWindow* Window = &Ctx->WindowMap[WindowIndex];

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

		for (int32 WidgetIndex = 0, WidgetCount = hmlen(Window->WidgetMap); WidgetIndex < WidgetCount; WidgetIndex++) {
			TGuiWidget* Widget = &Window->WidgetMap[WidgetIndex];

			Widget->OnRender(Ctx, Widget, Renderer);
		}
	}
}

// private implementations

void _TGuiResetActiveWindow(TGuiContext* Ctx)
{
	Ctx->ActiveWindow = NULL;
}

//   widgets
void TGuiWidgetCreateSubclass(TGuiContext* Ctx, TGuiWidget* Self)
{
	ASSERT(Ctx != NULL);
	ASSERT(Self != NULL);
	ASSERT(Self->SubClass == NULL);
	ASSERT(Self->OnCreate != NULL);
	Self->SubClass = Self->OnCreate(Ctx, Self);
}

void* TGuiButtonCreate(TGuiContext* Ctx, TGuiWidget* Super)
{
	TGuiButtonWidget* Self = (TGuiButtonWidget*)SDL_malloc(sizeof(TGuiButtonWidget));
	SDL_zerop(Self);
	return Self;
}

void TGuiButtonDestroy(TGuiContext* Ctx, TGuiWidget* Super)
{
	TGuiButtonWidget* Self = TGUI_WIDGET_CAST(TGuiButtonWidget, Super);
	SDL_free(Self);
}

void TGuiButtonNextFrame(TGuiContext* Ctx, TGuiWidget* Super)
{
	TGuiButtonWidget* Self = TGUI_WIDGET_CAST(TGuiButtonWidget, Super);
	Self->WasDown = Self->IsDown;
}

bool TGuiButtonProcessEvent(TGuiContext* Ctx, TGuiWidget* Super, const SDL_Event* Event)
{
	bool Result = false;

	if (Event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || Event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
		if (Event->button.button == 1) {
			TGuiButtonWidget* Self = TGUI_WIDGET_CAST(TGuiButtonWidget, Super);
			AABB ButtonBox = AABBFromTopLeftSize(Super->Position, Super->Size);
			Vec2 EventPos = V2(Event->button.x, Event->button.y);
			if (Event->button.down) {
				if (AABBContainsPoint(ButtonBox, EventPos)) {
					Self->IsDown = true;
					Result = true;
				}
			} else {
				Self->IsDown = true;
				if (AABBContainsPoint(ButtonBox, EventPos)) {
					Result = true;
				}
			}
		}
	}

	return Result;
}

void TGuiButtonRender(TGuiContext* Ctx, TGuiWidget* Super, SDL_Renderer* Renderer)
{
	TGuiButtonWidget* Self = TGUI_WIDGET_CAST(TGuiButtonWidget, Super);

	if (Self->IsDown) {
		SDL_SetRenderDrawColor(Renderer, 0, 200, 255, 255);
	} else {
		SDL_SetRenderDrawColor(Renderer, 0, 0, 255, 255);
	}

	Vec2 GlobalPos = Add(Super->OwningWindow->Position, Super->Position);

	SDL_RenderFillRect(
		Renderer,
		&(SDL_FRect){
			.x = GlobalPos.X,
			.y = GlobalPos.Y,
			.w = Super->Size.X,
			.h = Super->Size.Y,
		});
}
