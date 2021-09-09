/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <curses.h>

#include <unistd.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "boot_data.h"
#include "cbfs.h"
#include "ui_main.h"
#include "utils.h"

struct args
{
	const char *rom_file;
	const char *boot_order;
	const char **boot_options;
	int boot_option_count;
	bool interactive;
};

static const char *USAGE_FMT = "Usage: %s [-b boot-source,...] "
					 "[-o option=value] "
					 "[-h] "
					 "[-v] "
					 "coreboot.rom\n";

static bool run_ui(const struct args *args, struct boot_data *boot)
{
	WINDOW *window;
	bool save;

	initscr();
	noecho();
	curs_set(0);

	window = newwin(getmaxy(stdscr), getmaxx(stdscr), 0, 0);
	keypad(window, true);

	main_run(window, boot, args->rom_file, &save);

	delwin(window);

	endwin();

	/* Saving is performed after UI is turned off */
	if (save)
		return cbfs_store_boot_data(boot, args->rom_file);

	return true;
}

static bool batch_reorder(const struct args *args, struct boot_data *boot)
{
	int target = 0;
	char *ptr;
	const char *token;
	char *boot_order;

	if (args->boot_order == NULL)
		return true;

	boot_order = strdup(args->boot_order);

	for (ptr = boot_order; (token = strtok(ptr, ",")) != NULL; ptr = NULL) {
		int i;
		for (i = 0; i < boot->record_count; ++i) {
			if (strcmp(boot->records[i].name, token) == 0) {
				boot_data_move(boot, i, target++);
				break;
			}
		}

		if (i == boot->record_count) {
			fprintf(stderr, "Unrecognized boot record name: %s\n",
				token);
			break;
		}
	}

	free(boot_order);

	return (token == NULL);
}

static bool set_option(struct option *option, const char *str_value)
{
	int int_value = 0;
	const struct option_def *option_def = &OPTIONS[option->id];

	switch (option_def->type) {
		case OPT_TYPE_BOOLEAN:
			if (strcmp(str_value, "on") == 0)
				int_value = 1;
			else if (strcmp(str_value, "off") == 0)
				int_value = 0;
			else
				return false;
			break;
		case OPT_TYPE_TOGGLE:
			if (strcmp(str_value, "first") == 0)
				int_value = 0;
			else if (strcmp(str_value, "second") == 0)
				int_value = 1;
			else
				return false;
			break;
		case OPT_TYPE_HEX4:
			int_value = strtol(str_value, NULL, 10);
			break;
	}

	return boot_data_set_option(option, int_value);
}

static bool batch_set_options(const struct args *args, struct boot_data *boot)
{
	int i;

	for (i = 0; i < args->boot_option_count; ++i) {
		int n;
		int j;

		char name[64];
		char value[64];

		n = sscanf(args->boot_options[i], "%63[^=]=%63s", name, value);
		if (n != 2) {
			fprintf(stderr, "Unrecognized option setting: %s\n",
				args->boot_options[i]);
			break;
		}

		for (j = 0; j < boot->option_count; ++j) {
			const int id = boot->options[j].id;
			const struct option_def *option_def = &OPTIONS[id];
			if (strcmp(option_def->keyword, name) == 0) {
				break;
			}
		}

		if (j == boot->option_count) {
			fprintf(stderr, "Unrecognized option: %s\n", name);
			break;
		}

		if (!set_option(&boot->options[j], value)) {
			fprintf(stderr, "Invalid value for %s option: %s\n",
				name, value);
			break;
		}

	}

	return (i == args->boot_option_count);
}

static bool run_batch(const struct args *args, struct boot_data *boot)
{
	return batch_reorder(args, boot) &&
	       batch_set_options(args, boot) &&
	       cbfs_store_boot_data(boot, args->rom_file);
}

static void print_help(const char *command)
{
	size_t i;

	printf(USAGE_FMT, command);

	printf("\n");
	printf("boot-source is a value from a boot order list.\n");
	printf("\n");
	printf("Recognized options and possible values:\n");

	for (i = 0; i < ARRAY_SIZE(OPTIONS); ++i) {
		const struct option_def *option_def = &OPTIONS[i];
		printf("  %s\n", option_def->keyword);
		printf("    %s\n", option_def->description);

		switch (option_def->type) {
			case OPT_TYPE_BOOLEAN:
				printf("    on/off\n");
				break;
			case OPT_TYPE_TOGGLE:
				printf("    first/second\n");
				break;
			case OPT_TYPE_HEX4:
				printf("    [0; 65535]\n");
				break;
		}
	}
}

static const struct args *parse_args(int argc, char **argv)
{
	static struct args args;

	int i;
	int opt;

	while ((opt = getopt(argc, argv, "hvb:o:")) != -1) {
		switch (opt) {
			const char **option;

			case 'b':
				args.boot_order = optarg;
				break;
			case 'h':
				print_help(argv[0]);
				exit(EXIT_SUCCESS);
			case 'v':
				fprintf(stderr, "%s v%s\n",
					APP_NAME, APP_VERSION);
				exit(EXIT_SUCCESS);
			case 'o':
				option = GROW_ARRAY(args.boot_options,
						    args.boot_option_count);
				if (option != NULL) {
					*option = optarg;
					++args.boot_option_count;
				}
				break;

			case '?': /* parsing error */
				fprintf(stderr, USAGE_FMT, argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	/* positional arguments */
	for (i = optind; argv[i] != NULL; ++i) {
		if (args.rom_file != NULL) {
			fprintf(stderr, "Excessive positional argument: %s\n",
				argv[i]);
			exit(EXIT_FAILURE);
		}

		args.rom_file = argv[i];
	}

	if (args.rom_file == NULL) {
		fprintf(stderr, "ROM-file is missing from command line\n");
		fprintf(stderr, USAGE_FMT, argv[0]);
		exit(EXIT_FAILURE);
	}

	args.interactive = (args.boot_order == NULL) &&
			   (args.boot_option_count == 0);

	return &args;
}

int main(int argc, char **argv)
{
	struct boot_data *boot;
	bool success;

	const struct args *args = parse_args(argc, argv);

	boot = cbfs_load_boot_data(args->rom_file);
	if (boot == NULL) {
		fprintf(stderr, "Failed to read boot data\n");
		return EXIT_FAILURE;
	}

	if (args->interactive)
		success = run_ui(args, boot);
	else
		success = run_batch(args, boot);

	boot_data_free(boot);

	return (success ? EXIT_SUCCESS : EXIT_FAILURE);
}
