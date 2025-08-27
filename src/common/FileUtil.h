#pragma once

#include "Types.h"

// Allocates new buffer to read file into, returns true if succesfully read file and *OutFileData will point to the
// allocated buffer. The caller is responsible for freeing this buffer!
bool ReadFileToNewBuffer(const char* FileName, char** OutFileData);
