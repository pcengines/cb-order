#include <curses.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#include "boot_order.h"
#include "list_menu.h"

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))

char *format_str(const char format[], ...) __attribute__ ((format(gnu_printf, 1, 2)));
char *format_str(const char format[], ...)
{
	va_list ap;
	va_list aq;
	size_t len;
	char *buf;

	va_start(ap, format);
	va_copy(aq, ap);

	len = vsnprintf(NULL, 0, format, ap);
	va_end(ap);

	buf = malloc(len + 1);
	if (buf != NULL)
		(void)vsprintf(buf, format, aq);
	va_end(aq);

	return buf;
}

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

void run_options_menu(WINDOW *menu_window, struct boot_data *boot)
{
	int i;
	struct list_menu *options_menu;

	options_menu = list_menu_new("coreboot configuration :: options");

	for(i = 0; i < boot->option_count; ++i) {
		const enum option_id id = boot->options[i].id;
		const struct option_def *option = &OPTIONS[id];

		char *item = format_str("(%c)  %s",
					option->shortcut,
					option->description);
		list_menu_add_item(options_menu, item);
		free(item);
	}

	while (true) {
		const int key = list_menu_run(options_menu, menu_window);
		if (key == 'q')
			break;
	}

	list_menu_free(options_menu);
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
				run_options_menu(menu_window, boot);
				break;
		}
	}

	list_menu_free(main_menu);
}

int main(int argc, char **argv)
{
	struct boot_data *boot;
	WINDOW *menu_window;

	boot = boot_data_new();

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
