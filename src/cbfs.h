#ifndef CBFS_H__
#define CBFS_H__

#include <stdbool.h>

struct boot_data;

struct boot_data *cbfs_load_boot_data(const char *rom_file);
bool cbfs_store_boot_data(struct boot_data *boot, const char *rom_file);

#endif // CBFS_H__

/* vim: set ts=8 sts=8 sw=8 noet : */
