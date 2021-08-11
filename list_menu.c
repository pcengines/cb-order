#include "list_menu.h"

#include <curses.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

struct list_menu *list_menu_new(const char *title)
{
	struct list_menu *menu = malloc(sizeof(*menu));

	menu->title = strdup(title);
	if (menu->title == NULL) {
		free(menu);
		return NULL;
	}

	menu->item_count = 0;
	menu->items = NULL;
	menu->top = 0;
	menu->current = 0;

	return menu;
}

void list_menu_free(struct list_menu *menu)
{
	list_menu_clear(menu);

	free(menu->title);
	free(menu);
}

void list_menu_add_item(struct list_menu *menu, const char *item)
{
	char **new_item = GROW_ARRAY(menu->items, menu->item_count);
	if (new_item == NULL)
		return;

	*new_item = strdup(item);
	if (*new_item != NULL)
		++menu->item_count;
}

void list_menu_clear(struct list_menu *menu)
{
	int i;

	for (i = 0; i < menu->item_count; ++i)
		free(menu->items[i]);

	free(menu->items);

	menu->items = NULL;
	menu->item_count = 0;
}

void list_menu_goto(struct list_menu *menu, int index)
{
	if (index >= 0 && index < menu->item_count)
		menu->current = index;
}

static void adjust_viewport(struct list_menu *menu, int available_height)
{
	if (available_height >= menu->item_count)
		menu->top = 0;
	else if (menu->current < menu->top)
		menu->top = menu->current;
	else if (menu->current >= menu->top + available_height)
		menu->top = menu->current - available_height + 1;
}

void list_menu_draw(struct list_menu *menu, WINDOW *window)
{
	int i;
	/* Minus borders and padding */
	const int available_height = getmaxy(window) - 4;

	adjust_viewport(menu, available_height);

	werase(window);
	box(window, ACS_VLINE, ACS_HLINE);
	mvwprintw(window, 0, 2, " %s ", menu->title);

	for (i = 0; i < available_height; ++i) {
		const int item = menu->top + i;

		if (item >= menu->item_count)
			break;

		if (item == menu->current)
			wattron(window, A_REVERSE);

		mvwprintw(window, 2 + i, 2, " %s ", menu->items[item]);

		if (item == menu->current)
			wattroff(window, A_REVERSE);
	}

	wrefresh(window);
}

int list_menu_run(struct list_menu *menu, WINDOW *window)
{
	while (true) {
		int key;

		list_menu_draw(menu, window);

		key = wgetch(window);
		switch (key) {
			case 'k':
			case KEY_UP:
				if (menu->current > 0)
					--menu->current;
				break;
			case 'j':
			case KEY_DOWN:
				if (menu->current < menu->item_count - 1)
					++menu->current;
				break;
			case 'g':
			case KEY_HOME:
				menu->current = 0;
				break;
			case KEY_END:
				if (menu->item_count > 0)
					menu->current = menu->item_count - 1;
				break;
			default:
				return key;
		}
	}
}

/* vim: set ts=8 sts=8 sw=8 noet : */
