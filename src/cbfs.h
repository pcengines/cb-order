/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef CBFS_H__
#define CBFS_H__

#include <stdbool.h>

struct boot_data;

struct boot_data *cbfs_load_boot_data(const char *rom_file);
bool cbfs_store_boot_data(struct boot_data *boot, const char *rom_file);

#endif // CBFS_H__
