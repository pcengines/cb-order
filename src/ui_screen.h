/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef UI_SCREEN_H__
#define UI_SCREEN_H__

#include <curses.h>

#define CONTROL(key) ((key) & 0x1f)

struct screen
{
	char *title;

	int item_count;
	char **items;

	int hint_count;
	char **hints;

	int top;
	int current;
};

struct screen *screen_new(const char *title);
void screen_free(struct screen *screen);

void screen_add_item(struct screen *screen, const char *item);
void screen_clear_items(struct screen *screen);

void screen_add_hint(struct screen *screen, const char *hint);

void screen_goto(struct screen *screen, int index);
void screen_draw(struct screen *screen, WINDOW *window);
int screen_run(struct screen *screen, WINDOW *window);

#endif // UI_SCREEN_H__

/* vim: set ts=8 sts=8 sw=8 noet : */
