/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "ui_records.h"

#include <curses.h>

#include <stdlib.h>

#include "app.h"
#include "boot_data.h"
#include "ui_screen.h"
#include "utils.h"

static void fill_records_screen(struct screen *screen, struct boot_data *boot)
{
	int i;

	screen_clear_items(screen);

	for (i = 0; i < boot->record_count; ++i) {
		char *item = format_str("(%c)  %s",
					'A' + i,
					boot->records[i].name);
		screen_add_item(screen, item);
		free(item);
	}
}

void records_run(WINDOW *window, struct boot_data *boot)
{
	struct screen *screen;
	char *title;

	title = format_str("%s :: boot order", APP_TITLE);
	screen = screen_new(title);
	free(title);

	fill_records_screen(screen, boot);

	screen_add_hint(screen, "Down/j, Up/k        move cursor");
	screen_add_hint(screen, "Home/g, End         move cursor");
	screen_add_hint(screen, "PgDown/Ctrl+N       move record down");
	screen_add_hint(screen, "PgUp/Ctrl+P         move record up");
	screen_add_hint(screen, "(_)                 move record to current "
						    "position");
	screen_add_hint(screen, "Backspace/Left/q/h  leave");

	while (true) {
		const int key = screen_run(screen, window);
		if (key == ERR || key == 'q' || key == 'h' || key == KEY_LEFT ||
		    key == KEY_BACKSPACE || key == '\b')
			break;

		if (key >= 'A' && key < 'A' + boot->record_count) {
			const int item = key - 'A';
			const int line = screen->current;

			boot_data_move(boot, item, line);

			fill_records_screen(screen, boot);
			screen_goto(screen, line + 1);
		} else if (key == KEY_PPAGE || key == CONTROL('p')) {
			boot_data_move(boot,
				       screen->current,
				       screen->current - 1);
			fill_records_screen(screen, boot);
			screen_goto(screen, screen->current - 1);
		} else if (key == KEY_NPAGE || key == CONTROL('n')) {
			boot_data_move(boot,
				       screen->current,
				       screen->current + 1);
			fill_records_screen(screen, boot);
			screen_goto(screen, screen->current + 1);
		}
	}

	screen_free(screen);
}

/* vim: set ts=8 sts=8 sw=8 noet : */
