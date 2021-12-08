/* read and write binary file "partitions" described by FMAP */
/* SPDX-License-Identifier: GPL-2.0-only */

#define __BSD_VISIBLE 1

#include "partitioned_file.h"

#include "cbfs_sections.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>

struct partitioned_file {
	struct fmap *fmap;
	struct buffer buffer;
	FILE *stream;
};

static partitioned_file_t *reopen_flat_file(const char *filename,
					    bool write_access)
{
	assert(filename);
	struct partitioned_file *file = calloc(1, sizeof(*file));
	const char *access_mode;

	if (!file) {
		ERROR("Failed to allocate partitioned file structure\n");
		return NULL;
	}

	if (buffer_from_file(&file->buffer, filename)) {
		free(file);
		return NULL;
	}

	access_mode = write_access ?  "rb+" : "rb";
	file->stream = fopen(filename, access_mode);

	if (!file->stream || flock(fileno(file->stream), LOCK_EX)) {
		perror(filename);
		partitioned_file_close(file);
		return NULL;
	}

	return file;
}

partitioned_file_t *partitioned_file_reopen(const char *filename,
					    bool write_access)
{
	assert(filename);

	partitioned_file_t *file = reopen_flat_file(filename, write_access);
	if (!file)
		return NULL;

	long fmap_region_offset = fmap_find((const uint8_t *)file->buffer.data,
							file->buffer.size);
	if (fmap_region_offset < 0) {
		/* Supporting partitioned files only */
		partitioned_file_close(file);
		return NULL;
	}
	file->fmap = (struct fmap *)(file->buffer.data + fmap_region_offset);

	if (file->fmap->size > file->buffer.size) {
		int fmap_region_size = fmap_size(file->fmap);
		ERROR("FMAP records image size as %u, but file is only %zu bytes%s\n",
					file->fmap->size, file->buffer.size,
						fmap_region_offset == 0 &&
				(signed)file->buffer.size == fmap_region_size ?
				" (is it really an image, or *just* an FMAP?)" :
					" (did something truncate this file?)");
		partitioned_file_close(file);
		return NULL;
	}

	const struct fmap_area *fmap_fmap_entry =
				fmap_find_area(file->fmap, SECTION_NAME_FMAP);

	if (!fmap_fmap_entry) {
		partitioned_file_close(file);
		return NULL;
	}

	if ((long)fmap_fmap_entry->offset != fmap_region_offset) {
		ERROR("FMAP's '%s' section doesn't point back to FMAP start (did something corrupt this file?)\n",
							SECTION_NAME_FMAP);
		partitioned_file_close(file);
		return NULL;
	}

	return file;
}

bool partitioned_file_write_region(partitioned_file_t *file,
						const struct buffer *buffer)
{
	assert(file);
	assert(file->stream);
	assert(buffer);
	assert(buffer->data);

	if (buffer->data - buffer->offset != file->buffer.data) {
		ERROR("Attempted to write a partition buffer back to a different file than it came from\n");
		return false;
	}
	if (buffer->offset + buffer->size > file->buffer.size) {
		ERROR("Attempted to write data off the end of image file\n");
		return false;
	}

	if (fseek(file->stream, buffer->offset, SEEK_SET)) {
		ERROR("Failed to seek within image file\n");
		return false;
	}
	if (!fwrite(buffer->data, buffer->size, 1, file->stream)) {
		ERROR("Failed to write to image file\n");
		return false;
	}
	return true;
}

bool partitioned_file_read_region(struct buffer *dest,
			const partitioned_file_t *file, const char *region)
{
	assert(dest);
	assert(file);
	assert(file->buffer.data);
	assert(region);

	if (file->fmap) {
		const struct fmap_area *area = fmap_find_area(file->fmap,
									region);
		if (!area) {
			ERROR("Image is missing '%s' region\n", region);
			return false;
		}
		if (area->offset + area->size > file->buffer.size) {
			ERROR("Region '%s' runs off the end of the image file\n",
									region);
			return false;
		}
		buffer_splice(dest, &file->buffer, area->offset, area->size);
	} else {
		if (strcmp(region, SECTION_NAME_PRIMARY_CBFS) != 0) {
			ERROR("This is a legacy image that contains only a CBFS\n");
			return false;
		}
		buffer_clone(dest, &file->buffer);
	}

	return true;
}

void partitioned_file_close(partitioned_file_t *file)
{
	if (!file)
		return;

	file->fmap = NULL;
	buffer_delete(&file->buffer);
	if (file->stream) {
		flock(fileno(file->stream), LOCK_UN);
		fclose(file->stream);
		file->stream = NULL;
	}
	free(file);
}

static bool select_children_of(const struct fmap_area *child, const void *arg)
{
	assert(child);
	assert(arg);

	const struct fmap_area *parent = (const struct fmap_area *)arg;
	if (child == arg || (child->offset == parent->offset &&
						child->size == parent->size))
		return false;
	return child->offset >= parent->offset &&
		child->offset + child->size <= parent->offset + parent->size;
}
const partitioned_file_fmap_selector_t
		partitioned_file_fmap_select_children_of = select_children_of;
