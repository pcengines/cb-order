#include "boot_order.h"

#include <stdlib.h>

struct boot_data *boot_data_new(void)
{
	struct boot_data *boot = malloc(sizeof(*boot));

	boot->record_count = 0;
	boot->records = NULL;
	boot->option_count = 0;
	boot->options = NULL;

	return boot;
}

void boot_data_free(struct boot_data *boot)
{
	int i;

	for (i = 0; i < boot->record_count; ++i) {
		int j;
		struct boot_record *record = &boot->records[i];

		for (j = 0; j < record->device_count; ++j)
			free(record->devices[j]);

		free(record->devices);
		free(record->name);
		free(record);
	}

	free(boot->options);
	free(boot);
}

/* vim: set ts=8 sts=8 sw=8 noet : */
