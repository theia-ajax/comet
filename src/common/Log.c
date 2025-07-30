#include "Log.h"

#include <SDL3/SDL_stdinc.h>
#include <stdarg.h>
#include <stdio.h>

enum TermColor {
	TermColor_Normal,
	TermColor_Red,
	TermColor_Green,
	TermColor_Yellow,
	TermColor_Blue,
	TermColor_Magenta,
	TermColor_Cyan,
	TermColor_White,
	TermColor_LightGrey,
	TermColor_Count,
};

const char* LogLevelNames[] = {
	"NONE",
	"Error",
	"Warning",
	"Info",
	"Verbose",
	"<Disabled>",
};
_Static_assert(ARRAY_COUNT(LogLevelNames) == LogLevel_Count, "");

const enum TermColor LogLevelColors[] = {
	TermColor_Normal,
	TermColor_Red,
	TermColor_Yellow,
	TermColor_White,
	TermColor_LightGrey,
	TermColor_Normal,
};
_Static_assert(ARRAY_COUNT(LogLevelColors) == LogLevel_Count, "");

LogLevel GLogLevel = LogLevel_Disabled;

#ifdef LOGGING_WRITE_TO_FILE
FILE* GLogFile;
#endif

static void _InternalLogV(LogLevel Level, const char* Format, va_list Args);
static void _InternalSetTerminalColor(enum TermColor Color);

void LoggingInitialize(LogLevel Level)
{
	GLogLevel = Level;
#ifdef LOGGING_WRITE_TO_FILE
	GLogFile = fopen("log.txt", "w");
#endif

	LogInfo(__FUNCTION__);
}

void LoggingShutdown(void)
{
	LogInfo(__FUNCTION__);
	GLogLevel = LogLevel_None;
#ifdef LOGGING_WRITE_TO_FILE
	fclose(GLogFile);
#endif
	_InternalSetTerminalColor(TermColor_Normal);
}

void LoggingSetLogLevel(LogLevel Level)
{
	GLogLevel = Level;
}

void LogVerbose(const char* Format, ...)
{
	if (GLogLevel >= LogLevel_Verbose) {
		va_list Args;
		va_start(Args, Format);
		_InternalLogV(LogLevel_Verbose, Format, Args);
		va_end(Args);
	}
}

void LogInfo(const char* Format, ...)
{
	if (GLogLevel >= LogLevel_Info) {
		va_list Args;
		va_start(Args, Format);
		_InternalLogV(LogLevel_Info, Format, Args);
		va_end(Args);
	}
}

void LogWarning(const char* Format, ...)
{
	if (GLogLevel >= LogLevel_Warning) {
		va_list Args;
		va_start(Args, Format);
		_InternalLogV(LogLevel_Warning, Format, Args);
		va_end(Args);
	}
}

void LogError(const void* Format, ...)
{
	if (GLogLevel >= LogLevel_Error) {
		va_list Args;
		va_start(Args, Format);
		_InternalLogV(LogLevel_Error, Format, Args);
		va_end(Args);
	}
}

static void _InternalLogV(LogLevel Level, const char* Format, va_list Args)
{
	_InternalSetTerminalColor(LogLevelColors[Level]);

	FixedArray(char, 1024) Formatted;
	SDL_vsnprintf(Formatted.Data, FixedArrayCapacity(Formatted), Format, Args);

	FixedArray(char, 1024) Output;
	SDL_snprintf(Output.Data, FixedArrayCapacity(Output), "[%s] %s\n", LogLevelNames[Level], Formatted.Data);

	fprintf(stdout, Output.Data);
#ifdef LOGGING_WRITE_TO_FILE
	fprintf(GLogFile, Output.Data);
#endif
}

#include <SDL3/SDL_platform_defines.h>

// TODO: better cross-platform implementation (separate translation units per platform)
#if __LINUX__
const char* TermColorCodes[] = {
	"\x1B[0m",
	"\x1B[31m",
	"\x1B[32m",
	"\x1B[33m",
	"\x1B[34m",
	"\x1B[35m",
	"\x1B[36m",
	"\x1B[37m",
	"\x1B[37m",
};

_Static_assert(ARRAY_COUNT(TermColorCodes) == TermColor_Count, "");
#elif __WINDOWS__
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

const WORD TermColorCodes[] = {15, 4, 10, 14, 9, 13, 3, 15, 7};
_Static_assert(ARRAY_COUNT(TermColorCodes) == TermColor_Count, "");
// BLACK 0
// BLUE 1
// GREEN 2
// CYAN 3
// RED 4
// MAGENTA 5
// BROWN 6
// LIGHTGRAY 7
// DARKGRAY 8
// LIGHTBLUE 9
// LIGHTGREEN 10
// LIGHTCYAN 11
// LIGHTRED	12
// LIGHTMAGENTA	13
// YELLOW 14
// WHITE 15
#else
#error "Not Implemented Yet"
#endif

static void _InternalSetTerminalColor(enum TermColor Color)
{
	// TODO: better cross-platform implementation (separate translation units per platform)
#if __LINUX__
	ASSERT(VALID_INDEX(Color, TermColor_Count));
	printf("%s", TermColorCodes[Color]);
#elif __WINDOWS__
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, TermColorCodes[Color]);
#else
#error "Implement for other platforms"Color
#endif
}
