#include <cimgui.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef union SDL_Event SDL_Event;
typedef struct ImDrawData ImDrawData;

CIMGUI_API bool     ImGui_ImplSDL2_InitForOpenGL(SDL_Window* window, void* sdl_gl_context);
CIMGUI_API bool     ImGui_ImplSDL2_InitForVulkan(SDL_Window* window);
CIMGUI_API bool     ImGui_ImplSDL2_InitForD3D(SDL_Window* window);
CIMGUI_API bool     ImGui_ImplSDL2_InitForMetal(SDL_Window* window);
CIMGUI_API bool     ImGui_ImplSDL2_InitForSDLRenderer(SDL_Window* window, SDL_Renderer* renderer);
CIMGUI_API void     ImGui_ImplSDL2_Shutdown();
CIMGUI_API void     ImGui_ImplSDL2_NewFrame();
CIMGUI_API bool     ImGui_ImplSDL2_ProcessEvent(const SDL_Event* event);

CIMGUI_API bool     ImGui_ImplSDLRenderer_Init(SDL_Renderer* renderer);
CIMGUI_API void     ImGui_ImplSDLRenderer_Shutdown();
CIMGUI_API void     ImGui_ImplSDLRenderer_NewFrame();
CIMGUI_API void     ImGui_ImplSDLRenderer_RenderDrawData(ImDrawData* draw_data);

// Called by Init/NewFrame/Shutdown
CIMGUI_API bool     ImGui_ImplSDLRenderer_CreateFontsTexture();
CIMGUI_API void     ImGui_ImplSDLRenderer_DestroyFontsTexture();
CIMGUI_API bool     ImGui_ImplSDLRenderer_CreateDeviceObjects();
CIMGUI_API void     ImGui_ImplSDLRenderer_DestroyDeviceObjects();

#ifdef __cplusplus
}
#endif
