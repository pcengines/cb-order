/* common utility functions for cbfstool */
/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "common.h"
#include "cbfs.h"

/* Small, OS/libc independent runtime check for endianness */
int is_big_endian(void)
{
	static const uint32_t inttest = 0x12345678;
	const uint8_t inttest_lsb = *(const uint8_t *)&inttest;
	if (inttest_lsb == 0x12) {
		return 1;
	}
	return 0;
}

static off_t get_file_size(FILE *f)
{
	off_t fsize;
	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	return fsize;
}

int buffer_from_file(struct buffer *buffer, const char *filename)
{
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		perror(filename);
		return -1;
	}
	buffer->offset = 0;
	off_t file_size = get_file_size(fp);
	if (file_size < 0) {
		fprintf(stderr, "could not determine size of %s\n", filename);
		fclose(fp);
		return -1;
	}
	buffer->size = file_size;
	buffer->name = strdup(filename);
	buffer->data = (char *)malloc(buffer->size);
	assert(buffer->data);
	if (fread(buffer->data, 1, buffer->size, fp) != buffer->size) {
		fprintf(stderr, "incomplete read: %s\n", filename);
		fclose(fp);
		buffer_delete(buffer);
		return -1;
	}
	fclose(fp);
	return 0;
}

void buffer_delete(struct buffer *buffer)
{
	assert(buffer);
	if (buffer->name) {
		free(buffer->name);
		buffer->name = NULL;
	}
	if (buffer->data) {
		free(buffer_get_original_backing(buffer));
		buffer->data = NULL;
	}
	buffer->offset = 0;
	buffer->size = 0;
}
