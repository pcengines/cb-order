#include <curses.h>

#include <unistd.h> /* sleep() */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct list_menu
{
	char *title;

	int item_count;
	char **items;

	int current;
};

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

	for (i = 0; i < menu->item_count; ++i) {
		free(menu->items[i]);
	}

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

	for (i = 0; i < menu->item_count; ++i)
		mvwprintw(window, 1 + i, 2, " %s ", menu->items[i]);

	wrefresh(window);
}

void list_menu_run(struct list_menu *menu, WINDOW *window)
{
}

int main(int argc, char **argv)
{
	WINDOW *menu_window;
	struct list_menu *main_menu;

	initscr();
	noecho();
	curs_set(false);

	menu_window = newwin(getmaxy(stdscr), getmaxx(stdscr), 0, 0);

	main_menu = list_menu_new("coreboot configuration");
	list_menu_add_item(main_menu, "Edit boot order");
	list_menu_add_item(main_menu, "Edit options");
	list_menu_add_item(main_menu, "Exit");

	list_menu_draw(main_menu, menu_window);

	sleep(2);

	list_menu_free(main_menu);
	endwin();
}

/* vim: set ts=8 sts=8 sw=8 noet : */
