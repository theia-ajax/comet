#include "util.h"

#include "stb_ds.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t read_file_to_buffer(const char* filename, char** buf_ptr)
{
	FILE* fp = fopen(filename, "r");

	if (!fp) return 0;

	fseek(fp, 0, SEEK_END);
	size_t len = (size_t)ftell(fp);
	rewind(fp);

	char* buffer = (char*)malloc(len + 1);
	memset(buffer, 0, len + 1);

	char* curr = buffer;
	while (!feof(fp)) {
		*curr = fgetc(fp);
		curr++;
	}

	ptrdiff_t read_bytes = curr - buffer;

	buffer[len] = 0;

	*buf_ptr = buffer;

	return (size_t)read_bytes;
}

char* read_file(const char* filename, size_t* bytes_read)
{
	char* result = NULL;
	FILE* file = fopen(filename, "r");

	if (file != NULL) {
		fseek(file, 0, SEEK_END);
		long size = ftell(file);
		rewind(file);
		char* buffer = (char*)calloc(size + 1, 1);

		if (buffer != NULL) {
			size_t read = fread(buffer, 1, (size_t)size, file);
			if (bytes_read) *bytes_read = read;
			result = buffer;
		}

		fclose(file);
	}

	return result;
}

char** read_file_lines(const char* filename)
{
	char** result = NULL;
	arrsetcap(result, 256);

	size_t len;
	char* text = read_file(filename, &len);
	const char* delim = "\n";
	char* line = strtok(text, delim);
	while (line && *line > 0) {
		size_t line_len = strlen(line);
		char* copy = (char*)calloc(line_len + 1, 1);
		memcpy(copy, line, line_len);

		arrput(result, copy);

		line = strtok(NULL, delim);
	}
	free(text);

	return result;
}

void free_file_lines(char** lines)
{
	ptrdiff_t len = arrlen(lines);
	for (ptrdiff_t i = 0; i < len; ++i) {
		free(lines[i]);
	}
	arrfree(lines);
}

int floorf_to_int(float f)
{
	return (int)floorf(f);
}
