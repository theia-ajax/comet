#include "FileUtil.h"

#include <SDL3/SDL.h>

#include "Log.h"

bool ReadFileToNewBuffer(const char* FileName, char** OutFileData)
{
	SDL_assert(OutFileData !=  NULL);

	bool Success = false;
	*OutFileData = NULL;
	SDL_IOStream* ReadStream = SDL_IOFromFile(FileName, "r");
	do {
		if (!ReadStream) {
			break;
		}

		SDL_SeekIO(ReadStream, 0, SDL_IO_SEEK_END);
		size_t FileLength = (size_t)SDL_TellIO(ReadStream);
		SDL_SeekIO(ReadStream, 0, SDL_IO_SEEK_SET);

		char* FileData = (char*)SDL_calloc(FileLength + 1, sizeof(char));

		if (!FileData) {
			break;
		}

		size_t BytesRead = SDL_ReadIO(ReadStream, FileData, FileLength);

		LogInfo("Read %llu bytes from '%s', expected %llu", BytesRead, FileName, FileLength);

		*OutFileData = FileData;
		Success = true;
	} while (false);
	SDL_CloseIO(ReadStream);
}
