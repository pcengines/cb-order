#include "records.h"

#include <curses.h>

#include <stdlib.h>

#include "boot_data.h"
#include "list_menu.h"
#include "utils.h"

static void make_records_menu(struct list_menu *menu, struct boot_data *boot)
{
	int i;

	list_menu_clear_items(menu);

	for (i = 0; i < boot->record_count; ++i) {
		char *item = format_str("(%c)  %s",
					'A' + i,
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

	list_menu_add_hint(boot_menu, "Down/j, Up/k        move cursor");
	list_menu_add_hint(boot_menu, "Home/g, End         move cursor");
	list_menu_add_hint(boot_menu, "PgDown/Ctrl+N       move record down");
	list_menu_add_hint(boot_menu, "PgUp/Ctrl+P         move record up");
	list_menu_add_hint(boot_menu,
			   "(_)                 move record to current "
			                       "position");
	list_menu_add_hint(boot_menu, "Backspace/Left/q/h  leave menu");

	while (true) {
		const int key = list_menu_run(boot_menu, menu_window);
		if (key == ERR || key == 'q' || key == 'h' || key == KEY_LEFT ||
		    key == KEY_BACKSPACE || key == '\b')
			break;

		if (key >= 'A' && key < 'A' + boot->record_count) {
			const int item = key - 'A';
			const int line = boot_menu->current;

			boot_data_move(boot, item, line);

			make_records_menu(boot_menu, boot);
			list_menu_goto(boot_menu, line + 1);
		} else if (key == KEY_PPAGE || key == CONTROL('p')) {
			boot_data_move(boot,
				       boot_menu->current,
				       boot_menu->current - 1);
			make_records_menu(boot_menu, boot);
			list_menu_goto(boot_menu, boot_menu->current - 1);
		} else if (key == KEY_NPAGE || key == CONTROL('n')) {
			boot_data_move(boot,
				       boot_menu->current,
				       boot_menu->current + 1);
			make_records_menu(boot_menu, boot);
			list_menu_goto(boot_menu, boot_menu->current + 1);
		}
	}

	list_menu_free(boot_menu);
}

/* vim: set ts=8 sts=8 sw=8 noet : */
