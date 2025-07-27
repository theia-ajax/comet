#include <SDL3/SDL.h>
#include <stdlib.h>

#include "common/Application.h"

Application *GApp = NULL;

int main(int argc, char* argv[])
{
	Application* App = ApplicationInitialize(&(ApplicationConfig){});
	GApp = App;
	ApplicationRun(App);
	ApplicationShutdown(App);
	return 0;
}

SDL_NORETURN void PanicAndAbort(const char* Title, const char* Message)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, Title, Message, GetApplicationWindow(GApp));
	exit(1);
}