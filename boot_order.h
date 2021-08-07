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

	bool bool_value;
	int int_value;
};

struct boot_record
{
	char *name;

	int device_count;
	char **devices;
};

struct boot_file
{
	int record_count;
	struct boot_record *records;

	int option_count;
	struct option_def **options;
};

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
