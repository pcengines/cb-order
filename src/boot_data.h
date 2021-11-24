/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef BOOT_DATA_H__
#define BOOT_DATA_H__

#include <stdbool.h>
#include <stdio.h>

#define MAX_BOOT_RECORDS 64

enum option_type
{
	OPT_TYPE_BOOLEAN,
	OPT_TYPE_TOGGLE,
	OPT_TYPE_HEX4,
};

struct option_def
{
	const char *keyword;
	const char *description;
	char shortcut;
	enum option_type type;
	const char *toggle_options[2];
};

struct option
{
	// TODO: maybe use a pointer to option_def
	int id;
	int value;
};

struct boot_record
{
	char *name;

	int device_count;
	char **devices;
};

struct boot_data
{
	int record_count;
	struct boot_record *records;

	int option_count;
	struct option *options;

	/* Whether we use BOOTORDER region and not a CBFS file. */
	bool bootorder_region;
};

struct boot_data *boot_data_new(FILE *boot_file,
				FILE *map_file,
				bool bootorder_region);
void boot_data_free(struct boot_data *boot);

void boot_data_move(struct boot_data *boot, int from, int to);
bool boot_data_set_option(struct option *option, int value);

void boot_data_dump_boot(struct boot_data *boot, FILE *file);
void boot_data_dump_map(struct boot_data *boot, FILE *file);

static const struct option_def OPTIONS[] =
{
#define X(keyword_, description_, shortcut_, type_, ...) \
	{ \
		.keyword = (keyword_), \
		.description = (description_), \
		.shortcut = (shortcut_), \
		.type = (type_), \
		.toggle_options = { __VA_ARGS__ }, \
	},
#include "boot_options.inc"
#undef X
};

#endif // BOOT_DATA_H__
