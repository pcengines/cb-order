/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef UI_RECORDS_H__
#define UI_RECORDS_H__

#include <curses.h>

struct boot_data;

void records_run(WINDOW *window, struct boot_data *boot);

#endif // UI_RECORDS_H__

/* vim: set ts=8 sts=8 sw=8 noet : */
