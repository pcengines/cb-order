/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "cbfs.h"

#include <unistd.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

#include "boot_data.h"
#include "utils.h"

#include "third-party/cbfs_image.h"
#include "third-party/partitioned_file.h"

#define CBFS_REGION      "COREBOOT"
#define BOOTORDER_REGION "BOOTORDER"
#define BOOTORDER_FILE   "bootorder"
#define BOOTORDER_DEF    "bootorder_def"
#define BOOTORDER_MAP    "bootorder_map"

static FILE *read_from_rom(partitioned_file_t *pf,
			   const char *name,
			   bool is_region)
{
	FILE *fp;
	struct cbfs_file *entry;
	struct buffer region;
	struct cbfs_image cbfs;
	const char *region_name = (is_region ? name : CBFS_REGION);

	if (!partitioned_file_read_region(&region, pf, region_name)) {
		/* ERROR("The image will be left unmodified.\n"); */
		return NULL;
	}

	if(is_region) {
		FILE *fp = fmemopen(NULL, region.size, "w+");
		fwrite(region.data, 1, region.size, fp);
		rewind(fp);
		return fp;
	}

	if (cbfs_image_from_buffer(&cbfs, &region, ~0u) != 0)
		return NULL;

	entry = cbfs_get_entry(&cbfs, name);

	fp = fmemopen(NULL, ntohl(entry->len), "w+");
	fwrite(CBFS_SUBHEADER(entry), 1, ntohl(entry->len), fp);
	rewind(fp);
	return fp;
}

struct boot_data *cbfs_load_boot_data(const char *rom_file)
{
	FILE *boot_file;
	FILE *map_file;
	partitioned_file_t *pf;
	struct boot_data *boot = NULL;
	bool bootorder_region = true;

	pf = partitioned_file_reopen(rom_file, /*write_access=*/false);
	if (pf == NULL) {
		fprintf(stderr, "Failed to open ROM file for reading: %s\n",
			rom_file);
		goto failure;
	}

	boot_file = read_from_rom(pf, BOOTORDER_REGION, /*is_region=*/true);
	if (boot_file == NULL) {
		/* Use bootorder file if corresponding region is missing. */
		bootorder_region = false;
		boot_file = read_from_rom(pf, BOOTORDER_FILE,
					  /*is_region=*/false);
	}
	if (boot_file == NULL)
		goto failure;

	map_file = read_from_rom(pf, BOOTORDER_MAP, /*is_region=*/false);
	if (map_file == NULL) {
		(void)fclose(boot_file);
		goto failure;
	}

	boot = boot_data_new(boot_file, map_file, bootorder_region);

	(void)fclose(boot_file);
	(void)fclose(map_file);

failure:
	partitioned_file_close(pf);
	return boot;
}

static FILE *fopen_wrapper(const char *path, const char *mode)
{
	FILE *file = fopen(path, mode);

	if (file == NULL)
		fprintf(stderr, "Failed to open file %s: %s\n", path,
			strerror(errno));

	return file;
}

static bool fclose_wrapper(FILE *file, const char *path)
{
	if (fclose(file) != 0)
		fprintf(stderr,
			"Failed to close file %s: %s\n",
			path,
			strerror(errno));

	return file;
}

static bool pad_file(FILE *file)
{
	long size;
	int fill_amount;
	const int target_size = 4096;
	const char *pad_message = "this file needs to be 4096 bytes long in "
				  "order to entirely fill 1 spi flash sector";

	fseek(file, 0, SEEK_END);

	size = ftell(file);
	if (size > target_size - (long)strlen(pad_message)) {
		fprintf(stderr,
			"Boot file is greater than 4096 bytes: %ld\n", size);
		return false;
	}

	fill_amount = target_size - strlen(pad_message) - size;
	while (fill_amount-- > 0)
		fputc('\0', file);

	fputs(pad_message, file);

	return true;
}

static off_t get_file_size(FILE *fp)
{
	off_t fsize;
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	rewind(fp);
	return fsize;
}

static bool buffer_from_fp(struct buffer *buffer, FILE *fp)
{
	size_t nread;

	off_t file_size = get_file_size(fp);
	if (file_size < 0) {
		fprintf(stderr, "could not determine size of a file\n");
		(void)fclose_wrapper(fp, "<temporary>");
		return -1;
	}

	buffer->offset = 0;
	buffer->size = file_size;
	buffer->name = NULL;
	buffer->data = malloc(buffer->size);
	assert(buffer->data);

	nread = fread(buffer->data, 1, buffer->size, fp);
	if (nread != buffer->size) {
		fprintf(stderr, "Incomplete read of file: %lld out of %lld\n",
			(long long)nread, (long long)buffer->size);
		buffer_delete(buffer);
		return false;
	}

	return true;
}

static bool update_in_rom(partitioned_file_t *pf,
			  const char *name,
			  bool is_region,
			  FILE *fp)
{
	struct buffer region;
	struct buffer cbfs_file;
	struct cbfs_image cbfs;
	struct cbfs_file *file_header;
	const char *region_name = (is_region ? name : CBFS_REGION);

	rewind(fp);

	if (!partitioned_file_read_region(&region, pf, region_name)) {
		fprintf(stderr, "Failed to read ROM's region\n");
		return false;
	}

	if (is_region) {
		const size_t nread = fread(region.data, 1, region.size, fp);
		if (nread != region.size) {
			fprintf(stderr,
				"Incomplete read of file: %lld out of %lld\n",
				(long long)nread, (long long)region.size);
			return false;
		}

		return partitioned_file_write_region(pf, &region);
	}

	if (cbfs_image_from_buffer(&cbfs, &region, ~0u) != 0)
		return false;

	if (cbfs_remove_entry(&cbfs, name) != 0)
		return false;

	if (!buffer_from_fp(&cbfs_file, fp))
		return false;

	file_header =
		cbfs_create_file_header(CBFS_TYPE_RAW, cbfs_file.size, name);

	if (cbfs_add_entry(&cbfs, &cbfs_file, /*offset=*/0, file_header,
			   /*len_align=*/0) != 0)
		return false;

	return partitioned_file_write_region(pf, &region);
}

bool cbfs_store_boot_data(struct boot_data *boot, const char *rom_file)
{
	FILE *file = NULL;
	partitioned_file_t *pf = NULL;
	char template[] = "/tmp/cb-order.XXXXXX";
	const char *bootorder_name =
		(boot->bootorder_region ? BOOTORDER_REGION : BOOTORDER_FILE);

	/* Create temporary file which will be reused */

	file = temp_file(template);
	if (file == NULL) {
		fprintf(stderr, "Failed to create a temporary file: %s\n",
			strerror(errno));
		goto failure;
	}

	pf = partitioned_file_reopen(rom_file, /*write_access=*/true);
	if (pf == NULL) {
		fprintf(stderr, "Failed to open ROM file for writing: %s\n",
			rom_file);
		goto failure;
	}

	/* bootorder_def */

	boot_data_dump_boot(boot, file);
	if (!update_in_rom(pf, BOOTORDER_DEF, /*is_region=*/false, file))
		goto failure;

	/* bootorder */

	/* Same data, but padded */
	if (!pad_file(file))
		goto failure;
	if (!update_in_rom(pf, bootorder_name, boot->bootorder_region, file))
		goto failure;

	/* bootorder_map */

	if (!fclose_wrapper(file, template))
		goto failure;
	file = fopen_wrapper(template, "w+");
	if (file == NULL)
		goto failure;

	boot_data_dump_map(boot, file);
	if (!update_in_rom(pf, BOOTORDER_MAP, /*is_region=*/false, file))
		goto failure;

	if (!fclose_wrapper(file, template))
		goto failure;
	partitioned_file_close(pf);
	return true;

failure:
	if (file != 0)
		(void)fclose_wrapper(file, template);
	partitioned_file_close(pf);
	fprintf(stderr, "Updating ROM image has failed\n");
	return false;
}
