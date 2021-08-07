#include "boot_order.h"

#include <stdbool.h>
#include <stdlib.h>

#include "utils.h"

static void boot_data_add_option(struct boot_data *boot,
				 enum option_id id,
				 int value)
{
	struct option *new_option = GROW_ARRAY(boot->options,
					       boot->option_count);
	if (new_option == NULL)
		return;

	new_option->id = id;
	new_option->value = value;

	++boot->option_count;
}

struct boot_data *boot_data_new(void)
{
	struct boot_data *boot = malloc(sizeof(*boot));

	boot->record_count = 0;
	boot->records = NULL;
	boot->option_count = 0;
	boot->options = NULL;

	// XXX 
	boot_data_add_option(boot, PXEN, false);
	boot_data_add_option(boot, USBEN, true);
	boot_data_add_option(boot, SCON, false);
	boot_data_add_option(boot, COM2EN, true);
	boot_data_add_option(boot, UARTC, true);
	boot_data_add_option(boot, UARTD, false);
	boot_data_add_option(boot, MPCIE2_CLK, false);
	boot_data_add_option(boot, EHCIEN, true);
	boot_data_add_option(boot, BOOSTEN, false);
	boot_data_add_option(boot, WATCHDOG, 123);
	boot_data_add_option(boot, SD3MODE, true);
	boot_data_add_option(boot, PCIEREVERSE, false);
	boot_data_add_option(boot, IOMMU, true);
	boot_data_add_option(boot, PCIEPM, false);

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
