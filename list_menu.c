#include "list_menu.h"

#include <curses.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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
	menu->current = 0;

	return menu;
}

void list_menu_free(struct list_menu *menu)
{
	int i;

	for (i = 0; i < menu->item_count; ++i)
		free(menu->items[i]);

	free(menu->items);
	free(menu->title);
	free(menu);
}

void list_menu_add_item(struct list_menu *menu, const char *item)
{
	char *item_copy = strdup(item);
	if (item_copy == NULL)
		return;

	char **items = realloc(menu->items,
			       sizeof(*menu->items)*(menu->item_count + 1));
	if (items == NULL) {
		free(item_copy);
		return;
	}

	menu->items = items;
	menu->items[menu->item_count] = item_copy;
	++menu->item_count;
}

void list_menu_draw(struct list_menu *menu, WINDOW *window)
{
	int i;

	werase(window);
	box(window, ACS_VLINE, ACS_HLINE);
	mvwprintw(window, 0, 2, " %s ", menu->title);

	for (i = 0; i < menu->item_count; ++i) {
		if (i == menu->current)
			wattron(window, A_REVERSE);

		mvwprintw(window, 2 + i, 2, " %s ", menu->items[i]);

		if (i == menu->current)
			wattroff(window, A_REVERSE);
	}

	wrefresh(window);
}

static void list_menu_go_up(struct list_menu *menu)
{
	if (menu->current > 0)
		--menu->current;
}

static void list_menu_go_down(struct list_menu *menu)
{
	if (menu->current < menu->item_count - 1)
		++menu->current;
}

int list_menu_run(struct list_menu *menu, WINDOW *window)
{
	while (true) {
		int key;

		list_menu_draw(menu, window);

		key = wgetch(window);
		switch (key) {
			case KEY_UP:
				list_menu_go_up(menu);
				break;
			case KEY_DOWN:
				list_menu_go_down(menu);
				break;
			default:
				return key;
		}
	}
}

/* vim: set ts=8 sts=8 sw=8 noet : */
