#pragma once

#include "Types.h"

void LoggingInitialize(void);
void LoggingShutdown(void);

void LogInfo(const char* Format, ...);
void LogWarning(const char* Format, ...);
void LogError(const void* Format, ...);