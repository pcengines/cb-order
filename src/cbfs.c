/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "cbfs.h"

#include <unistd.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

#include "boot_data.h"
#include "utils.h"

#define BOOTORDER_REGION "BOOTORDER"
#define BOOTORDER_DEF    "bootorder_def"
#define BOOTORDER_MAP    "bootorder_map"

static FILE *extract(const char *cbfs_tool,
		     const char *rom_file,
		     const char *name,
		     bool is_region)
{
	FILE *file;
	char template[] = "/tmp/cb-order.XXXXXX";
	char *file_argv[] = {
		(char *)cbfs_tool, (char *)rom_file, "extract",
		"-n", (char *)name,
		"-f", template,
		NULL
	};
	char *region_argv[] = {
		(char *)cbfs_tool, (char *)rom_file, "read",
		"-r", (char *)name,
		"-f", template,
		NULL
	};
	char **argv = (is_region ? region_argv : file_argv);

	file = temp_file(template);
	if (file == NULL) {
		fprintf(stderr,
			"Failed to create a temporary file: %s\n",
			strerror(errno));
		return NULL;
	}

	if (!run_cmd(argv)) {
		(void)fclose(file);
		(void)unlink(template);

		fprintf(stderr,
			"Failed to extract %s from %s\n",
			name, rom_file);
		return NULL;
	}

	(void)unlink(template);
	return file;
}

struct boot_data *cbfs_load_boot_data(const char *cbfs_tool,
				      const char *rom_file)
{
	FILE *boot_file;
	FILE *map_file;
	struct boot_data *boot;

	boot_file = extract(cbfs_tool,
			    rom_file,
			    BOOTORDER_REGION,
			    /*is_region=*/true);
	if (boot_file == NULL)
		return NULL;
	map_file = extract(cbfs_tool,
			   rom_file,
			   BOOTORDER_MAP,
			   /*is_region=*/false);
	if (map_file == NULL) {
		(void)fclose(boot_file);
		return NULL;
	}

	boot = boot_data_new(boot_file, map_file);

	fclose(boot_file);
	fclose(map_file);

	return boot;
}

static FILE *fopen_wrapper(const char *path, const char *mode)
{
	FILE *file = fopen(path, mode);

	if (file == NULL)
		fprintf(stderr,
			"Failed to open file %s: %s\n",
			path,
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
			"Boot file is greater than 4096 bytes: %ld", size);
		return false;
	}

	fill_amount = target_size - strlen(pad_message) - size;
	while (fill_amount-- > 0)
		fputc('\0', file);

	fputs(pad_message, file);

	return true;
}

bool cbfs_store_boot_data(const char *cbfs_tool,
			  struct boot_data *boot,
			  const char *rom_file)
{
	FILE *file;
	bool success;
	char template[] = "/tmp/cb-order.XXXXXX";
	char name[128] = BOOTORDER_DEF;
	char *add_argv[] = {
		(char *)cbfs_tool, (char *)rom_file, "add",
		"-t", "raw",
		"-n", name,
		"-a", "0x1000",
		"-f", template,
		NULL
	};
	char *remove_argv[] = {
		(char *)cbfs_tool, (char *)rom_file, "remove",
		"-n", name,
		NULL
	};
	char *bootorder_argv[] = {
		(char *)cbfs_tool, (char *)rom_file, "write",
		"-r", BOOTORDER_REGION,
		"-f", template,
		NULL
	};

	file = temp_file(template);
	if (file == NULL) {
		fprintf(stderr,
			"Failed to create a temporary file: %s\n",
			strerror(errno));
		return false;
	}

	/* bootorder_def */

	boot_data_dump_boot(boot, file);
	if (!fclose_wrapper(file, template))
		goto failure;
	if (!run_cmd(remove_argv) || !run_cmd(add_argv))
		goto failure;

	/* bootorder */

	file = fopen_wrapper(template, "w");
	if (file == NULL)
		goto failure;
	boot_data_dump_boot(boot, file);
	success = pad_file(file);
	if (!fclose_wrapper(file, template) || !success)
		goto failure;
	if (!run_cmd(bootorder_argv))
		goto failure;

	/* bootorder_map */

	strcpy(name, BOOTORDER_MAP);

	file = fopen_wrapper(template, "w");
	if (file == NULL)
		goto failure;
	boot_data_dump_map(boot, file);
	if (!fclose_wrapper(file, template))
		goto failure;
	if (!run_cmd(remove_argv) || !run_cmd(add_argv))
		goto failure;

	(void)unlink(template);
	return true;

failure:
	(void)unlink(template);
	return false;
}
