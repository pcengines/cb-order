#include <curses.h>

#include <stdbool.h>
#include <stdio.h>

#include "boot_order.h"
#include "list_menu.h"
#include "options.h"

void run_boot_menu(WINDOW *menu_window, struct boot_data *boot)
{
	struct list_menu *boot_menu;

	boot_menu = list_menu_new("coreboot configuration :: boot order");
	list_menu_add_item(boot_menu, "(a)  USB");
	list_menu_add_item(boot_menu, "(b)  SDCARD");
	list_menu_add_item(boot_menu, "(c)  mSATA");
	list_menu_add_item(boot_menu, "(d)  SATA");
	list_menu_add_item(boot_menu, "(e)  mPCIe1 SATA1 and SATA2");
	list_menu_add_item(boot_menu, "(f)  iPXE");

	while (true) {
		const int key = list_menu_run(boot_menu, menu_window);
		if (key == 'q')
			break;
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
		const int key = list_menu_run(main_menu, menu_window);
		if (key == 'q')
			break;

		switch (key) {
			case 'b':
				run_boot_menu(menu_window, boot);
				break;
			case 'o':
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
	FILE *file;

	file = fopen("bootorder", "r");
	boot = boot_data_new(file);
	fclose(file);

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
