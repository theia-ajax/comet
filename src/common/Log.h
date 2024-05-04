#pragma once

#include "Types.h"

typedef enum LogLevel {
	LogLevel_None,
	LogLevel_Error,
	LogLevel_Warning,
	LogLevel_Info,
	LogLevel_Verbose,
	LogLevel_Disabled, // By being the highest level no log function can ever log if level is set to diabled.
	LogLevel_Count,
} LogLevel;

void LoggingInitialize(LogLevel Level);
void LoggingShutdown(void);
void LoggingSetLogLevel(LogLevel Level);

void LogVerbose(const char* Format, ...);
void LogInfo(const char* Format, ...);
void LogWarning(const char* Format, ...);
void LogError(const void* Format, ...);