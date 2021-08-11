/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "boot_data.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

static void boot_data_add_device(struct boot_record *record, const char *device)
{
	char **new_device = GROW_ARRAY(record->devices, record->device_count);
	if (new_device == NULL)
		return;

	*new_device = strdup(device);
	if (*new_device == NULL)
		return;

	++record->device_count;
}

static void boot_data_add_record(struct boot_data *boot, const char *name)
{
	struct boot_record *new_record = GROW_ARRAY(boot->records,
						    boot->record_count);
	if (new_record == NULL)
		return;

	new_record->name = strdup(name);
	if (new_record->name == NULL)
		return;

	new_record->device_count = 0;
	new_record->devices = NULL;

	++boot->record_count;
}

static void boot_data_add_option(struct boot_data *boot, int id, int value)
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
	int i;
	for (i = 0; i < boot->option_count; ++i) {
		struct option *option = &boot->options[i];
		const struct option_def *option_def = &OPTIONS[option->id];
		const char *option_line = line;

		int base;

		if (!skip_prefix(&option_line, option_def->keyword))
			continue;

		base = (option_def->type == OPT_TYPE_HEX4 ? 16 : 10);
		option->value = strtol(option_line, NULL, base);
		return;
	}

	fprintf(stderr, "Failed to parse option line: %s\n", line);
}

static void strip(char *line)
{
	size_t line_len = strlen(line);

	if (line_len > 0 && line[line_len - 1] == '\n')
		--line_len;
	if (line_len > 0 && line[line_len - 1] == '\r')
		--line_len;
	line[line_len] = '\0';
}

static void boot_data_parse_map(struct boot_data *boot,
				FILE *map_file,
				char *record_sizes)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	char last_record = '\0';

	while ((read = getline(&line, &len, map_file)) != -1) {
		if (line[0] == '\0')
			break;

		strip(line);
		if (strlen(line) < 3) {
			fprintf(stderr, "Ignoring invalid map line: %s\n",
				line);
			continue;
		}

		if (line[0] != last_record) {
			if (boot->record_count == MAX_BOOT_RECORDS) {
				fprintf(stderr,
					"Ignoring excess records starting "
					"with: %s\n",
					line);
				break;
			}

			boot_data_add_record(boot, line + 2);
			last_record = line[0];
		}

		++record_sizes[boot->record_count - 1];
	}

	free(line);
}

static void boot_data_parse(struct boot_data *boot,
			    FILE *boot_file,
			    FILE *map_file)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	char record_sizes[MAX_BOOT_RECORDS] = {0};
	int current_record = 0;

	boot_data_parse_map(boot, map_file, record_sizes);

	while ((read = getline(&line, &len, boot_file)) != -1) {
		struct boot_record *record;

		if (line[0] == '\0')
			break;

		strip(line);
		if (line[0] != '/') {
			boot_data_parse_option(boot, line);
			continue;
		}

		if (current_record == boot->record_count) {
			fprintf(stderr,
				"Ignoring invalid boot line (invalid map?): %s",
				line);
			break;
		}

		record = &boot->records[current_record];
		boot_data_add_device(record, line);

		if (record->device_count == record_sizes[current_record])
			++current_record;
	}

	free(line);
}

struct boot_data *boot_data_new(FILE *boot_file, FILE *map_file)
{
	size_t i;
	struct boot_data *boot = malloc(sizeof(*boot));

	boot->record_count = 0;
	boot->records = NULL;
	boot->option_count = 0;
	boot->options = NULL;

	for (i = 0; i < ARRAY_SIZE(OPTIONS); ++i)
		boot_data_add_option(boot, i, /*value=*/0);

	boot_data_parse(boot, boot_file, map_file);

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
	}

	free(boot->options);
	free(boot);
}

void boot_data_move(struct boot_data *boot, int from, int to)
{
	if (from < 0 || from >= boot->record_count)
		return;
	if (to < 0 || to >= boot->record_count)
		return;

	if (from >= to)
		ROTATE_RIGHT(&boot->records[to], from - to + 1);
	else
		ROTATE_LEFT(&boot->records[from], to - from + 1);
}

bool boot_data_set_option(struct option *option, int value)
{
	const struct option_def *option_def = &OPTIONS[option->id];
	if (option_def->type != OPT_TYPE_HEX4) {
		option->value = (value != 0);
		return true;
	}

	if (value < 0 || value > 0xffff)
		return false;

	option->value = value;
	return true;
}

void boot_data_dump_boot(struct boot_data *boot, FILE *file)
{
	int i;

	for (i = 0; i < boot->record_count; ++i) {
		int j;
		for (j = 0; j < boot->records[i].device_count; ++j)
			fprintf(file, "%s\r\n", boot->records[i].devices[j]);
	}

	for (i = 0; i < boot->option_count; ++i) {
		const int id = boot->options[i].id;
		const struct option_def *option_def = &OPTIONS[id];

		if (option_def->type == OPT_TYPE_HEX4)
			fprintf(file, "%s%04x\r\n",
				option_def->keyword,
				boot->options[i].value);
		else
			fprintf(file, "%s%d\r\n",
				option_def->keyword,
				boot->options[i].value);
	}
}

void boot_data_dump_map(struct boot_data *boot, FILE *file)
{
	int i;
	for (i = 0; i < boot->record_count; ++i) {
		int j;
		for (j = 0; j < boot->records[i].device_count; ++j)
			fprintf(file, "%c %s\r\n",
				'a' + i,
				boot->records[i].name);
	}
}

/* vim: set ts=8 sts=8 sw=8 noet : */
