#include <curses.h>

#include <unistd.h> /* sleep() */

#include <stdbool.h>

#include "list_menu.h"

void run_boot_menu(WINDOW *menu_window)
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

void run_options_menu(WINDOW *menu_window)
{
	struct list_menu *options_menu;

	options_menu = list_menu_new("coreboot configuration :: options");
	list_menu_add_item(options_menu, "(n)  Network/PXE boot");
	list_menu_add_item(options_menu, "(u)  USB boot");
	list_menu_add_item(options_menu, "(t)  Serial console");
	list_menu_add_item(options_menu, "(k)  Redirect console output to COM2");
	list_menu_add_item(options_menu, "(o)  UART C / GPIO[0..7]");
	list_menu_add_item(options_menu, "(p)  UART D / GPIO[10..17]");
	list_menu_add_item(options_menu, "(m)  Force mPCIe2 slot CLK (GPP3 PCIe)");
	list_menu_add_item(options_menu, "(h)  EHCI0 controller");
	list_menu_add_item(options_menu, "(l)  Core Performance Boost");
	list_menu_add_item(options_menu, "(i)  Watchdog");
	list_menu_add_item(options_menu, "(j)  SD 3.0 mode");
	list_menu_add_item(options_menu, "(g)  Reverse order of PCI addresses");
	list_menu_add_item(options_menu, "(v)  IOMMU");
	list_menu_add_item(options_menu, "(y)  PCIe power management features");
	list_menu_add_item(options_menu, "(w)  Enable BIOS write protect");

	while (true) {
		const int key = list_menu_run(options_menu, menu_window);
		if (key == 'q')
			break;
	}

	list_menu_free(options_menu);
}

void run_main_menu(WINDOW *menu_window)
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
				run_boot_menu(menu_window);
				break;
			case 'o':
				run_options_menu(menu_window);
				break;
		}
	}

	list_menu_free(main_menu);
}

int main(int argc, char **argv)
{
	WINDOW *menu_window;

	initscr();
	noecho();
	curs_set(false);

	menu_window = newwin(getmaxy(stdscr), getmaxx(stdscr), 0, 0);
	keypad(menu_window, true);

	run_main_menu(menu_window);

	delwin(menu_window);

	endwin();
}

/* vim: set ts=8 sts=8 sw=8 noet : */
