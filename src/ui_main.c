/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "ui_main.h"

#include <curses.h>

#include "app.h"
#include "boot_data.h"
#include "ui_options.h"
#include "ui_records.h"
#include "ui_screen.h"
#include "utils.h"

void main_run(WINDOW *window,
	      struct boot_data *boot,
	      const char *rom_file,
	      bool *save)
{
	struct screen *screen;
	char *title;

	title = format_str("%s :: %s", APP_TITLE, rom_file);
	screen = screen_new(title);
	free(title);

	screen_add_item(screen, "(B)  Edit boot order");
	screen_add_item(screen, "(O)  Edit options");
	screen_add_item(screen, "(S)  Save & Exit");
	screen_add_item(screen, "(X)  Exit");

	screen_add_hint(screen, "Down/j, Up/k          move cursor");
	screen_add_hint(screen, "Home/g, End           move cursor");
	screen_add_hint(screen, "Enter/Right/l/(key)  run current item");

	*save = false;

	while (true) {
		int key = screen_run(screen, window);

		if (key == '\n' || key == 'l' || key == KEY_RIGHT)
			key = "BOSX"[screen->current];

		if (key == ERR || key == 'X' || key == 'S') {
			*save = (key == 'S');
			break;
		}

		switch (key) {
			case 'B':
				screen_goto(screen, 0);
				records_run(window, boot);
				break;
			case 'O':
				screen_goto(screen, 1);
				options_run(window, boot);
				break;
		}
	}

	screen_free(screen);
}
