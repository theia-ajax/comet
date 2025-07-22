#include "StringId.h"

#define STRPOOL_U32 uint32
#define STPROOL_U64 uint64
#define STRPOOL_IMPLEMENTATION
#include "strpool.h"

#include "Log.h"

struct {
	bool IsInitialized;
	strpool_t Pool;
} GStringId;

StringId GetStringId(const char* string)
{
	return GetStringIdN(string, (string) ? strlen(string) : 0);
}

StringId GetStringIdN(const char* string, size_t length)
{
	StringId Result = {
		.Id = strpool_inject(&GStringId.Pool, string, length),
	};
#if _DEBUG
	Result.DebugString = StringIdCStr(Result);
#endif
	return Result;
}

const char* StringIdCStr(StringId stringId)
{
	return strpool_cstr(&GStringId.Pool, stringId.Id);
}

bool StringIdIsValid(StringId S)
{
	return strpool_cstr(&GStringId.Pool, S.Id) != NULL;
}

bool StringIdEq(StringId A, StringId B)
{
	return A.Id == B.Id;
}

void StringIdInvalidate(StringId* Id)
{
	ASSERT(Id != NULL);
	Id->Id = KRawInvalidStringId;
}

void StringIdPoolsInitialize(void)
{
	ASSERT(!GStringId.IsInitialized);

	strpool_config_t Config = strpool_default_config;
	Config.index_bits = 32;
	Config.counter_bits = 0;

	strpool_init(&GStringId.Pool, &Config);

	GStringId.IsInitialized = true;

	LogInfo(__FUNCTION__);
}

void StringIdPoolsShutdown(void)
{
	ASSERT(GStringId.IsInitialized);

	strpool_term(&GStringId.Pool);

	GStringId.IsInitialized = false;

	LogInfo(__FUNCTION__);
}
