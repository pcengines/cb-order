#ifndef RECORDS_H__
#define RECORDS_H__

#include <curses.h>

struct boot_data;

void records_menu_run(WINDOW *menu_window, struct boot_data *boot);

#endif // RECORDS_H__

/* vim: set ts=8 sts=8 sw=8 noet : */
