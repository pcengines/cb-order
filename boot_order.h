#ifndef BOOTORDER_H__
#define BOOTORDER_H__

#include <stdbool.h>
#include <stdio.h>

#define MAX_BOOT_RECORDS 64

enum option_type
{
	OPT_TYPE_BOOLEAN,
	OPT_TYPE_TOGGLE,
	OPT_TYPE_HEX4,
};

enum option_id
{
#define X(id_, keyword_, description_, shortcut_, type_) id_,
#include "boot_options.inc"
#undef X
};

struct option_def
{
	const char *keyword;
	const char *description;
	char shortcut;
	enum option_type type;
};

struct option
{
	// TODO: maybe use a pointer to option_def
	enum option_id id;
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
};

struct boot_data *boot_data_new(FILE *boot_file, FILE *map_file);
void boot_data_free(struct boot_data *boot);

void boot_data_move(struct boot_data *boot, int from, int to);
bool boot_data_set_option(struct option *option, int value);

void boot_data_dump_boot(struct boot_data *boot, FILE *file);
void boot_data_dump_map(struct boot_data *boot, FILE *file);

static const struct option_def OPTIONS[] =
{
#define X(id_, keyword_, description_, shortcut_, type_) \
	{ \
		.keyword = (keyword_), \
		.description = (description_), \
		.shortcut = (shortcut_), \
		.type = (type_), \
	},
#include "boot_options.inc"
#undef X
};

#endif // BOOTORDER_H__

/* vim: set ts=8 sts=8 sw=8 noet : */
