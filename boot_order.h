#ifndef BOOTORDER_H__
#define BOOTORDER_H__

#include <stdbool.h>

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
};

struct option
{
	const struct option_def *def;
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
	struct option **options;
};

struct boot_data *boot_data_new(void);
void boot_data_free(struct boot_data *boot);

static const struct option_def OPTIONS[] =
{
#define X(keyword_, description_, shortcut_, type_) \
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
