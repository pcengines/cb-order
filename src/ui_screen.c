/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "ui_screen.h"

#include <curses.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define MAX_WIDTH  80
#define MAX_HEIGHT 30

struct screen *screen_new(const char *title)
{
	struct screen *screen = malloc(sizeof(*screen));

	screen->title = strdup(title);
	if (screen->title == NULL) {
		free(screen);
		return NULL;
	}

	screen->item_count = 0;
	screen->items = NULL;

	screen->hint_count = 0;
	screen->hints = NULL;

	screen->top = 0;
	screen->current = 0;

	return screen;
}

void screen_free(struct screen *screen)
{
	int i;

	screen_clear_items(screen);

	for (i = 0; i < screen->hint_count; ++i)
		free(screen->hints[i]);
	free(screen->hints);

	free(screen->title);
	free(screen);
}

void screen_add_item(struct screen *screen, const char *item)
{
	char **new_item = GROW_ARRAY(screen->items, screen->item_count);
	if (new_item == NULL)
		return;

	*new_item = strdup(item);
	if (*new_item != NULL)
		++screen->item_count;
}

void screen_clear_items(struct screen *screen)
{
	int i;

	for (i = 0; i < screen->item_count; ++i)
		free(screen->items[i]);

	free(screen->items);

	screen->items = NULL;
	screen->item_count = 0;
}

void screen_add_hint(struct screen *screen, const char *hint)
{
	char **new_item = GROW_ARRAY(screen->hints, screen->hint_count);
	if (new_item == NULL)
		return;

	*new_item = strdup(hint);
	if (*new_item != NULL)
		++screen->hint_count;
}

void screen_goto(struct screen *screen, int index)
{
	if (index >= 0 && index < screen->item_count)
		screen->current = index;
}

static void adjust_viewport(struct screen *screen, int available_height)
{
	if (available_height >= screen->item_count)
		screen->top = 0;
	else if (screen->current < screen->top)
		screen->top = screen->current;
	else if (screen->current >= screen->top + available_height)
		screen->top = screen->current - available_height + 1;
}

void screen_draw(struct screen *screen, WINDOW *window)
{
	int i;
	const int term_height = getmaxy(stdscr);
	const int term_width = getmaxx(stdscr);
	const int h = (term_height > MAX_HEIGHT ? MAX_HEIGHT : term_height);
	const int w = (term_width > MAX_WIDTH ? MAX_WIDTH : term_width);
	/* Minus borders and padding */
	const int available_height = h - 4;

	/* Limit maximum size and center working area */
	wresize(window, h, w);
	mvwin(window, (term_height - h)/2, (term_width - w)/2);

	werase(stdscr);
	werase(window);
	/*
	 * Tell curses to ignore any assumptions about the state of the screen.
	 * This is to fix UI glitches on less capable terminals.
	 */
	redrawwin(stdscr);
	redrawwin(window);

	box(window, ACS_VLINE, ACS_HLINE);
	mvwprintw(window, 0, 2, " %s ", screen->title);

	adjust_viewport(screen, available_height);

	for (i = 0; i < available_height; ++i) {
		const int item = screen->top + i;

		if (item >= screen->item_count)
			break;

		if (item == screen->current)
			wattron(window, A_REVERSE);

		mvwprintw(window, 2 + i, 2, "%s", screen->items[item]);

		if (item == screen->current)
			wattroff(window, A_REVERSE);
	}

	if (available_height >= screen->item_count + 1 + screen->hint_count) {
		for (i = 0; i < screen->hint_count; ++i) {
			const int line = 2 + screen->item_count + 1 + i;
			mvwprintw(window, line, 2, "%s", screen->hints[i]);
		}
	}

	wnoutrefresh(stdscr);
	wnoutrefresh(window);
	doupdate();
}

int screen_run(struct screen *screen, WINDOW *window)
{
	while (true) {
		int key;

		screen_draw(screen, window);

		key = wgetch(window);
		switch (key) {
			case 'k':
			case KEY_UP:
				if (screen->current > 0)
					--screen->current;
				break;
			case 'j':
			case KEY_DOWN:
				if (screen->current < screen->item_count - 1)
					++screen->current;
				break;
			case 'g':
			case KEY_HOME:
				screen->current = 0;
				break;
			case KEY_END:
				if (screen->item_count > 0)
					screen->current = screen->item_count - 1;
				break;
			default:
				return key;
		}
	}
}
