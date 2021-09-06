/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef UI_MAIN_H__
#define UI_MAIN_H__

#include <curses.h>

struct boot_data;

void main_run(WINDOW *window,
              struct boot_data *boot,
              const char *rom_file,
              bool *save);

#endif // UI_MAIN_H__
