#include <curses.h>

#include <unistd.h>

#include <stdbool.h>
#include <stdio.h>

#include "boot_order.h"
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
		   const char *rom_file)
{
	struct list_menu *main_menu;
	char *title;

	title = format_str("coreboot configuration :: %s", rom_file);
	main_menu = list_menu_new(title);
	free(title);

	list_menu_add_item(main_menu, "(b)  Edit boot order");
	list_menu_add_item(main_menu, "(o)  Edit options");
	list_menu_add_item(main_menu, "(q)  Exit");

	while (true) {
		int key = list_menu_run(main_menu, menu_window);

		if (key == '\n')
			key = "boq"[main_menu->current];

		if (key == ERR || key == 'q')
			break;

		switch (key) {
			case 'b':
				list_menu_goto(main_menu, 0);
				records_menu_run(menu_window, boot);
				break;
			case 'o':
				list_menu_goto(main_menu, 1);
				options_menu_run(menu_window, boot);
				break;
		}
	}

	list_menu_free(main_menu);
}

static void run_ui(const char *rom_file, struct boot_data *boot)
{
	WINDOW *menu_window;

	initscr();
	noecho();
	curs_set(false);

	menu_window = newwin(getmaxy(stdscr), getmaxx(stdscr), 0, 0);
	keypad(menu_window, true);

	run_main_menu(menu_window, boot, rom_file);

	delwin(menu_window);

	endwin();
}

static void run_batch(const struct args *args, struct boot_data *boot)
{

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
		run_ui(args->rom_file, boot);
	else
		run_batch(args, boot);

	success = cbfs_store_boot_data(boot, args->rom_file);

	boot_data_free(boot);

	return (success ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* vim: set ts=8 sts=8 sw=8 noet : */
