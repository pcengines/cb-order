/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef UI_OPTIONS_H__
#define UI_OPTIONS_H__

#include <curses.h>

struct boot_data;

void options_run(WINDOW *window, struct boot_data *boot);

#endif // UI_OPTIONS_H__

/* vim: set ts=8 sts=8 sw=8 noet : */
