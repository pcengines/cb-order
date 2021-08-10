#include "records.h"

#include <curses.h>

#include <stdlib.h>

#include "boot_data.h"
#include "list_menu.h"
#include "utils.h"

static void make_records_menu(struct list_menu *menu, struct boot_data *boot)
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

void records_menu_run(WINDOW *menu_window, struct boot_data *boot)
{
	struct list_menu *boot_menu;

	boot_menu = list_menu_new("coreboot configuration :: boot order");

	make_records_menu(boot_menu, boot);

	while (true) {
		const int key = list_menu_run(boot_menu, menu_window);
		if (key == ERR || key == 'q')
			break;

		if (key >= 'a' && key < 'a' + boot->record_count) {
			const int item = key - 'a';
			const int line = boot_menu->current;

			boot_data_move(boot, item, line);

			make_records_menu(boot_menu, boot);
			list_menu_goto(boot_menu, line + 1);
		}
	}

	list_menu_free(boot_menu);
}

/* vim: set ts=8 sts=8 sw=8 noet : */
