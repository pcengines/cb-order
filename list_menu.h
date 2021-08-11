#ifndef LIST_MENU_H__
#define LIST_MENU_H__

#include <curses.h>

#define CONTROL(key) ((key) & 0x1f)

struct list_menu
{
	char *title;

	int item_count;
	char **items;

	int hint_count;
	char **hints;

	int top;
	int current;
};

struct list_menu *list_menu_new(const char *title);
void list_menu_free(struct list_menu *menu);

void list_menu_add_item(struct list_menu *menu, const char *item);
void list_menu_clear_items(struct list_menu *menu);

void list_menu_add_hint(struct list_menu *menu, const char *hint);

void list_menu_goto(struct list_menu *menu, int index);
void list_menu_draw(struct list_menu *menu, WINDOW *window);
int list_menu_run(struct list_menu *menu, WINDOW *window);

#endif // LIST_MENU_H__

/* vim: set ts=8 sts=8 sw=8 noet : */
