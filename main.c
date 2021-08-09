#include <curses.h>

#include <stdbool.h>
#include <stdio.h>

#include "boot_order.h"
#include "list_menu.h"
#include "options.h"
#include "utils.h"

static void make_boot_menu(struct list_menu *menu, struct boot_data *boot)
{
	int i;

	list_menu_clear(menu);

	for (i = 0; i < boot->record_count; ++i) {
		char *item = format_str("(%c)  %s",
					'a' + i,
					boot->records[i].name);
		list_menu_add_item(menu, item);
		free(item);
	}
}

void run_boot_menu(WINDOW *menu_window, struct boot_data *boot)
{
	struct list_menu *boot_menu;

	boot_menu = list_menu_new("coreboot configuration :: boot order");

	make_boot_menu(boot_menu, boot);

	while (true) {
		const int key = list_menu_run(boot_menu, menu_window);
		if (key == ERR || key == 'q')
			break;

		if (key >= 'a' && key < 'a' + boot->record_count) {
			const int item = key - 'a';
			const int line = boot_menu->current;

			if (item >= line)
				ROTATE_RIGHT(&boot->records[line],
					     item - line + 1);
			else
				ROTATE_LEFT(&boot->records[item],
					    line - item + 1);

			make_boot_menu(boot_menu, boot);
			list_menu_goto(boot_menu, line + 1);
		}
	}

	list_menu_free(boot_menu);
}

void run_main_menu(WINDOW *menu_window, struct boot_data *boot)
{
	struct list_menu *main_menu;

	main_menu = list_menu_new("coreboot configuration");
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
				run_boot_menu(menu_window, boot);
				break;
			case 'o':
				list_menu_goto(main_menu, 1);
				options_menu_run(menu_window, boot);
				break;
		}
	}

	list_menu_free(main_menu);
}

int main(int argc, char **argv)
{
	struct boot_data *boot;
	WINDOW *menu_window;
	FILE *boot_file;
	FILE *map_file;

	boot_file = fopen("bootorder", "r");
	map_file = fopen("bootorder_map", "r");
	boot = boot_data_new(boot_file, map_file);
	fclose(map_file);
	fclose(boot_file);

	initscr();
	noecho();
	curs_set(false);

	menu_window = newwin(getmaxy(stdscr), getmaxx(stdscr), 0, 0);
	keypad(menu_window, true);

	run_main_menu(menu_window, boot);

	delwin(menu_window);

	endwin();

	boot_data_free(boot);
}

/* vim: set ts=8 sts=8 sw=8 noet : */
