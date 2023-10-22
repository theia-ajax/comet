#pragma once

#include "types.h"

typedef struct StringId {
	uint32 Id;
#if _DEBUG
	const char* DebugString;
#endif
} StringId;

#define KStringIdInvalid (StringId){0}

#define STR_ID_LITERAL(str) GetStringIdN(str, sizeof(len))

StringId GetStringId(const char* string);
StringId GetStringIdN(const char* string, size_t length);
const char* StringIdCStr(StringId stringId);

void StringIdPoolsInitialize(void);
void StringIdPoolsShutdown(void);
