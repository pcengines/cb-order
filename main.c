#include <curses.h>

#include <stdbool.h>
#include <stdio.h>

#include "boot_order.h"
#include "cbfs.h"
#include "list_menu.h"
#include "records.h"
#include "options.h"
#include "utils.h"

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

int main(int argc, char **argv)
{
	const char *rom_file;
	struct boot_data *boot;
	bool success;

	if (argc != 2) {
		fprintf(stderr, "Invocation: %s <rom-file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	rom_file = argv[1];

	boot = cbfs_load_boot_data(rom_file);
	if (boot == NULL) {
		fprintf(stderr, "Failed to read boot data\n");
		return EXIT_FAILURE;
	}

	run_ui(rom_file, boot);

	success = cbfs_store_boot_data(boot, rom_file);

	boot_data_free(boot);

	return (success ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* vim: set ts=8 sts=8 sw=8 noet : */
