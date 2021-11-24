/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "ui_options.h"

#include <curses.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "boot_data.h"
#include "ui_screen.h"
#include "utils.h"

static char *format_option_item(struct option *option)
{
	const struct option_def *option_def = &OPTIONS[option->id];

	char *value = NULL;
	char *line = NULL;

	const char *first = (option_def->toggle_options[0] == NULL)
			  ? "first"
			  : option_def->toggle_options[0];
	const char *second = (option_def->toggle_options[1] == NULL)
			  ? "second"
			  : option_def->toggle_options[1];

	switch (option_def->type) {
		case OPT_TYPE_BOOLEAN:
			value = strdup(option->value ? "on" : "off");
			break;
		case OPT_TYPE_TOGGLE:
			value = strdup(option->value ? first : second);
			break;
		case OPT_TYPE_HEX4:
			value = format_str("%d", option->value);
			break;
	}

	line = format_str("(%c)  [%-11s = %6s]  %s",
			  option_def->shortcut,
			  option_def->keyword,
			  value,
			  option_def->description);

	free(value);
	return line;
}

static void fill_options_screen(struct screen *screen, struct boot_data *boot)
{
	int i;

	screen_clear_items(screen);

	for (i = 0; i < boot->option_count; ++i) {
		char *item = format_option_item(&boot->options[i]);
		screen_add_item(screen, item);
		free(item);
	}
}

static char *get_number(WINDOW *window,
			const char *title,
			const char *description,
			const char *prompt,
			const char *initial)
{
	char input_buf[128];
	bool done = false;
	bool accepted = false;
	int visibility;
	size_t pos;

	snprintf(input_buf, sizeof(input_buf), "%s", initial);
	pos = strlen(input_buf);

	visibility = curs_set(1);

	while (!done) {
		int key;

		werase(window);
		box(window, ACS_VLINE, ACS_HLINE);
		mvwprintw(window, 0, 2, " %s ", title);

		mvwprintw(window, 2, 2,
			  "%s%s", prompt, input_buf);
		mvwprintw(window, 4, 2, "%s", description);

		wmove(window, 2, 2 + strlen(prompt) + pos);
		wrefresh(window);

		key = wgetch(window);
		switch (key) {
			case '\x08': /* Ctrl+H */
			case KEY_BACKSPACE:
				if (pos > 0)
					input_buf[--pos] = '\0';
				break;
			case '\x15': /* Ctrl+U */
				input_buf[0] = '\0';
				pos = 0;
				break;
			case '\n':
			case KEY_ENTER:
				done = true;
				accepted = true;
				break;
			case '\x1b': /* Escape */
				done = true;
				break;
			default:
				if (isdigit(key) && pos < sizeof(input_buf)) {
					input_buf[pos++] = key;
					input_buf[pos] = '\0';
				}
				break;
		}
	}

	curs_set(visibility);

	if (!accepted)
		return NULL;

	return strdup(input_buf);
}

static void toggle_option(struct option *option, WINDOW *window)
{
	char *title;
	char *input;
	const struct option_def *option_def = &OPTIONS[option->id];

	if (option_def->type != OPT_TYPE_HEX4) {
		option->value = !option->value;
		return;
	}

	if (option->value != 0) {
		option->value = 0;
		return;
	}

	title = format_str("%s :: options :: %s",
			   APP_TITLE,
			   option_def->description);
	input = get_number(window, title,
			   "Range: [0; 65535]", "New value: ", "");
	free(title);

	if (input != NULL) {
		(void)boot_data_set_option(option, strtol(input, NULL, 10));
		free(input);
	}
}

void options_run(WINDOW *window, struct boot_data *boot)
{
	struct screen *screen;

	screen = screen_new("coreboot configuration :: options");
	fill_options_screen(screen, boot);

	screen_add_hint(screen, "Down/j, Up/k             move cursor");
	screen_add_hint(screen, "Home/g, End              move cursor");
	screen_add_hint(screen, "Space/Enter/Right/l/(_)  toggle/set option");
	screen_add_hint(screen, "Backspace/Left/q/h       leave");

	while (true) {
		int i;

		const int key = screen_run(screen, window);
		if (key == ERR || key == 'q' || key == 'h' || key == KEY_LEFT ||
		    key == KEY_BACKSPACE || key == '\b')
			break;

		if (key == ' ' || key == '\n' || key == 'l' ||
		    key == KEY_RIGHT) {
			toggle_option(&boot->options[screen->current], window);
			fill_options_screen(screen, boot);
			continue;
		}

		for (i = 0; i < boot->option_count; ++i) {
			struct option *option = &boot->options[i];
			const int id = option->id;
			const struct option_def *option_def = &OPTIONS[id];

			if (option_def->shortcut == key) {
				toggle_option(option, window);
				fill_options_screen(screen, boot);
				screen_goto(screen, i);
				break;
			}
		}
	}

	screen_free(screen);
}
