#pragma once

#include "Types.h"

typedef struct StringId {
	uint32 Id;
#if _DEBUG
	const char* DebugString;
#endif
} StringId;

enum { KRawInvalidStringId = 0 };

#define KInvalidStringId ((StringId){KRawInvalidStringId})

#define STR_ID_LITERAL(str) GetStringIdN(str, sizeof(len))

StringId GetStringId(const char* string);
StringId GetStringIdN(const char* string, size_t length);
const char* StringIdCStr(StringId stringId);
bool StringIdIsValid(StringId S);
bool StringIdEq(StringId A, StringId B);
void StringIdInvalidate(StringId* Id);

void StringIdPoolsInitialize(void);
void StringIdPoolsShutdown(void);
