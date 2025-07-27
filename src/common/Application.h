#pragma once

typedef struct ApplicationConfig {

} ApplicationConfig;

typedef struct Application Application;
typedef struct SDL_Window SDL_Window;

Application* ApplicationInitialize(const ApplicationConfig* Config);
void ApplicationShutdown(Application *App);
void ApplicationRun(Application *App);

SDL_Window *GetApplicationWindow(Application *App);