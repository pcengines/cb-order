#include "boot_order.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

static bool skip_prefix(const char **str, const char *prefix)
{
	const size_t prefix_len = strlen(prefix);
	if (strncmp(*str, prefix, prefix_len) == 0) {
		*str += prefix_len;
		return true;
	}
	return false;
}

static void boot_data_add_option(struct boot_data *boot,
				 enum option_id id,
				 int value)
{
	struct option *new_option = GROW_ARRAY(boot->options,
					       boot->option_count);
	if (new_option == NULL)
		return;

	new_option->id = id;
	new_option->value = value;

	++boot->option_count;
}

static void boot_data_parse_option(struct boot_data *boot, const char *line)
{
	size_t i;
	for (i = 0; i < ARRAY_SIZE(OPTIONS); ++i) {
		const struct option_def *option_def = &OPTIONS[i];
		const char *option_line = line;

		int base;
		int value;

		if (!skip_prefix(&option_line, option_def->keyword))
			continue;

		base = (option_def->type == OPT_TYPE_HEX4 ? 16 : 10);
		value = strtol(option_line, NULL, base);
		boot_data_add_option(boot, i, value);
		return;
	}

	fprintf(stderr, "Failed to parse option line: %s", line);
}

static void boot_data_parse(struct boot_data *boot, FILE *file)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	while ((read = getline(&line, &len, file)) != -1) {
		if (line[0] == '\0')
			break;

		if (line[0] != '/')
			boot_data_parse_option(boot, line);
	}

	free(line);
}

static void boot_data_add_missing_options(struct boot_data *boot)
{
	size_t i;
	for (i = 0; i < ARRAY_SIZE(OPTIONS); ++i) {
		int j;
		for (j = 0; j < boot->option_count; ++j) {
			if (boot->options[j].id == i)
				break;
		}

		if (j == boot->option_count)
			boot_data_add_option(boot, i, /*value=*/0);
	}
}

struct boot_data *boot_data_new(FILE *file)
{
	struct boot_data *boot = malloc(sizeof(*boot));

	boot->record_count = 0;
	boot->records = NULL;
	boot->option_count = 0;
	boot->options = NULL;

	boot_data_parse(boot, file);
	boot_data_add_missing_options(boot);

	return boot;
}

void boot_data_free(struct boot_data *boot)
{
	int i;

	for (i = 0; i < boot->record_count; ++i) {
		int j;
		struct boot_record *record = &boot->records[i];

		for (j = 0; j < record->device_count; ++j)
			free(record->devices[j]);

		free(record->devices);
		free(record->name);
		free(record);
	}

	free(boot->options);
	free(boot);
}

/* vim: set ts=8 sts=8 sw=8 noet : */
