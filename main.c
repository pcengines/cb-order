#include <curses.h>

#include <unistd.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "boot_data.h"
#include "cbfs.h"
#include "list_menu.h"
#include "records.h"
#include "options.h"
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
					 "[-o option=value] [-h]\n";

void run_main_menu(WINDOW *menu_window,
		   struct boot_data *boot,
		   const char *rom_file,
		   bool *save)
{
	struct list_menu *main_menu;
	char *title;

	title = format_str("coreboot configuration :: %s", rom_file);
	main_menu = list_menu_new(title);
	free(title);

	list_menu_add_item(main_menu, "(B)  Edit boot order");
	list_menu_add_item(main_menu, "(O)  Edit options");
	list_menu_add_item(main_menu, "(S)  Save & Exit");
	list_menu_add_item(main_menu, "(X)  Exit");

	list_menu_add_hint(main_menu, "Down/j, Up/k       move cursor");
	list_menu_add_hint(main_menu, "Home/g, End        move cursor");
	list_menu_add_hint(main_menu, "Enter/Right/l/(_)  run menu item");

	*save = false;

	while (true) {
		int key = list_menu_run(main_menu, menu_window);

		if (key == '\n' || key == 'l' || key == KEY_RIGHT)
			key = "BOSX"[main_menu->current];

		if (key == ERR || key == 'X' || key == 'S') {
			*save = (key == 's');
			break;
		}

		switch (key) {
			case 'B':
				list_menu_goto(main_menu, 0);
				records_menu_run(menu_window, boot);
				break;
			case 'O':
				list_menu_goto(main_menu, 1);
				options_menu_run(menu_window, boot);
				break;
		}
	}

	list_menu_free(main_menu);
}

static bool run_ui(const char *rom_file, struct boot_data *boot)
{
	WINDOW *menu_window;
	bool save;

	initscr();
	noecho();
	curs_set(false);

	menu_window = newwin(getmaxy(stdscr), getmaxx(stdscr), 0, 0);
	keypad(menu_window, true);

	run_main_menu(menu_window, boot, rom_file, &save);

	delwin(menu_window);

	endwin();

	/* Saving is performed after UI is turned off */
	if (save)
		return cbfs_store_boot_data(boot, rom_file);

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
	int int_value;
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
		size_t j;

		char *name = NULL;
		char *value = NULL;

		n = sscanf(args->boot_options[i], "%m[^=]=%ms", &name, &value);
		if (n != 2) {
			free(name);
			free(value);
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

		if (j == ARRAY_SIZE(OPTIONS)) {
			fprintf(stderr, "Unrecognized option: %s\n", name);
			free(name);
			break;
		}

		if (!set_option(&boot->options[j], value)) {
			fprintf(stderr, "Invalid value for %s option: %s\n",
				name, value);
			free(value);
			free(name);
			break;
		}

		free(value);
		free(name);
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

	int opt;

	while ((opt = getopt(argc, argv, "-hb:o:")) != -1) {
		switch (opt) {
			const char **option;

			case 'b':
				args.boot_order = optarg;
				break;
			case 'h':
				print_help(argv[0]);
				exit(EXIT_SUCCESS);
			case 'o':
				option = GROW_ARRAY(args.boot_options,
						    args.boot_option_count);
				if (option != NULL) {
					*option = optarg;
					++args.boot_option_count;
				}
				break;

			case 1: /* positional argument */
				if (args.rom_file != NULL) {
					fprintf(stderr, "Excess positional "
							"argument: %s\n",
						argv[optind - 1]);
					exit(EXIT_FAILURE);
				}
				args.rom_file = argv[optind - 1];
				break;

			case '?': /* parsing error */
				fprintf(stderr, USAGE_FMT, argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if (args.rom_file == NULL) {
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
		success = run_ui(args->rom_file, boot);
	else
		success = run_batch(args, boot);

	boot_data_free(boot);

	return (success ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* vim: set ts=8 sts=8 sw=8 noet : */
