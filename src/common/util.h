#pragma once

#include "types.h"
#include <math.h>
#include <string.h>

size_t read_file_to_buffer(const char *filename, char **buf_ptr);
char *read_file(const char *filename, size_t *bytes_read);
char **read_file_lines(const char *filename);
void free_file_lines(char **lines);

int floorf_to_int(float f);
