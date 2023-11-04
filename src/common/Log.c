#include "Log.h"

#include <SDL2/SDL_stdinc.h>
#include <stdarg.h>
#include <stdio.h>

typedef enum LogLevel { LogLevel_None, LogLevel_Error, LogLevel_Warning, LogLevel_Info, LogLevel_Count } LogLevel;

const char* LogLevelNames[] = {
	"NONE",
	"Error",
	"Warning",
	"Info",
};
_Static_assert(ARRAY_COUNT(LogLevelNames) == LogLevel_Count, "");

LogLevel GLogLevel;
FILE* GLogFile;

static void _InternalLogV(LogLevel Level, const char* Format, va_list Args);

void LoggingInitialize(void)
{
	GLogLevel = LogLevel_Info;
	GLogFile = fopen("log.txt", "w");

	LogInfo(__FUNCTION__);
}

void LoggingShutdown(void)
{
	LogInfo(__FUNCTION__);
	GLogLevel = LogLevel_None;
	fclose(GLogFile);
}

void LogInfo(const char* Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	_InternalLogV(LogLevel_Info, Format, Args);
	va_end(Args);
}

void LogWarning(const char* Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	_InternalLogV(LogLevel_Warning, Format, Args);
	va_end(Args);
}

void LogError(const void* Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	_InternalLogV(LogLevel_Error, Format, Args);
	va_end(Args);
}

static void _InternalLogV(LogLevel Level, const char* Format, va_list Args)
{
	if (GLogLevel >= Level) {
		FixedArray(char, 1024) Formatted;
		SDL_vsnprintf(Formatted.Data, FixedArrayCapacity(Formatted), Format, Args);

		FixedArray(char, 1024) Output;
		SDL_snprintf(Output.Data, FixedArrayCapacity(Output), "[%s] %s\n", LogLevelNames[Level], Formatted.Data);

		fprintf(stdout, Output.Data);
		fprintf(GLogFile, Output.Data);
	}
}
