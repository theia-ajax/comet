#pragma once

typedef struct ApplicationConfig {

} ApplicationConfig;

typedef struct Application Application;

Application* ApplicationInitialize(const ApplicationConfig* Config);
void ApplicationShutdown(Application *App);

void ApplicationRun(Application *App);