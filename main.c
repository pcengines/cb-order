#include <curses.h>

#include <unistd.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "boot_order.h"
#include "list_menu.h"
#include "records.h"
#include "options.h"
#include "utils.h"

void run_main_menu(WINDOW *menu_window,
		   struct boot_data *boot,
		   const char *rom_file)
{
	struct list_menu *main_menu;
	char *title;

	title = format_str("coreboot configuration :: %s", rom_file);
	main_menu = list_menu_new(title);
	free(title);

	list_menu_add_item(main_menu, "(b)  Edit boot order");
	list_menu_add_item(main_menu, "(o)  Edit options");
	list_menu_add_item(main_menu, "(q)  Exit");

	while (true) {
		int key = list_menu_run(main_menu, menu_window);

		if (key == '\n')
			key = "boq"[main_menu->current];

		if (key == ERR || key == 'q')
			break;

		switch (key) {
			case 'b':
				list_menu_goto(main_menu, 0);
				records_menu_run(menu_window, boot);
				break;
			case 'o':
				list_menu_goto(main_menu, 1);
				options_menu_run(menu_window, boot);
				break;
		}
	}

	list_menu_free(main_menu);
}

static bool pad_file(FILE *file)
{
	long size;
	int fill_amount;
	const int target_size = 4096;
	const char *pad_message = "this file needs to be 4096 bytes long in "
				  "order to entirely fill 1 spi flash sector";

	fseek(file, 0, SEEK_END);

	size = ftell(file);
	if (size > target_size - strlen(pad_message)) {
		fprintf(stderr,
			"Boot file is greater than 4096 bytes: %ld", size);
		return false;
	}

	fill_amount = target_size - strlen(pad_message) - size;
	while (fill_amount-- > 0)
		fputc('\0', file);

	fputs(pad_message, file);

	return true;
}

static FILE *fopen_wrapper(const char *path, const char *mode)
{
	FILE *file = fopen(path, mode);

	if (file == NULL)
		fprintf(stderr,
			"Failed to open file %s: %s\n",
			path,
			strerror(errno));

	return file;
}

static bool fclose_wrapper(FILE *file, const char *path)
{
	if (fclose(file) != 0)
		fprintf(stderr,
			"Failed to close file %s: %s\n",
			path,
			strerror(errno));

	return file;
}

static bool store_boot_data(struct boot_data *boot,
			    const char *rom_file,
			    const char *boot_def_file_path,
			    const char *boot_file_path,
			    const char *map_file_path)
{
	FILE *file;
	bool success;
	char template[] = "/tmp/cb-order.XXXXXX";
	char name[] = "bootorder_def";
	char *add_argv[] = {
		"cbfstool", (char *)rom_file, "add",
		"-t", "raw",
		"-n", name,
		"-a", "0x1000",
		"-f", template,
		NULL
	};
	char *remove_argv[] = {
		"cbfstool", (char *)rom_file, "remove",
		"-n", name,
		NULL
	};
	char *bootorder_argv[] = {
		"cbfstool", (char *)rom_file, "write",
		"-r", "BOOTORDER",
		"-f", template,
		NULL
	};

	file = temp_file(template);
	if (file == NULL) {
		fprintf(stderr,
			"Failed to create a temporary file: %s\n",
			strerror(errno));
		return false;
	}

	boot_data_dump_boot(boot, file);
	if (!fclose_wrapper(file, template))
		goto failure;
	if (!run_cmd(remove_argv))
		goto failure;
	if (!run_cmd(add_argv))
		goto failure;

	file = fopen_wrapper(template, "w");
	if (file == NULL)
		goto failure;
	boot_data_dump_boot(boot, file);
	success = pad_file(file);
	if (!fclose_wrapper(file, template) || !success)
		goto failure;
	if (!run_cmd(bootorder_argv))
		goto failure;

	strcpy(name, "bootorder_map");

	file = fopen_wrapper(template, "w");
	if (file == NULL)
		goto failure;
	boot_data_dump_map(boot, file);
	if (!fclose_wrapper(file, template))
		goto failure;
	if (!run_cmd(remove_argv))
		goto failure;
	if (!run_cmd(add_argv))
		goto failure;

	(void)unlink(template);
	return true;

failure:
	(void)unlink(template);
	return false;
}

static FILE *extract_file(const char *rom_file, const char *name)
{
	FILE *file;
	char template[] = "/tmp/cb-order.XXXXXX";
	char *argv[] = {
		"cbfstool", (char *)rom_file, "extract",
		"-n", (char *)name,
		"-f", template,
		NULL
	};

	file = temp_file(template);
	if (file == NULL) {
		fprintf(stderr,
			"Failed to create a temporary file: %s\n",
			strerror(errno));
		return NULL;
	}

	if (!run_cmd(argv)) {
		(void)fclose(file);
		(void)unlink(template);

		fprintf(stderr,
			"Failed to extract %s from %s\n",
			name, rom_file);
		return NULL;
	}

	(void)unlink(template);
	return file;
}

static struct boot_data *load_boot_data(const char *rom_file)
{
	FILE *boot_file;
	FILE *map_file;
	struct boot_data *boot;

	boot_file = extract_file(rom_file, "bootorder_def");
	if (boot_file == NULL)
		return NULL;
	map_file = extract_file(rom_file, "bootorder_map");
	if (map_file == NULL) {
		(void)fclose(boot_file);
		return NULL;
	}

	boot = boot_data_new(boot_file, map_file);

	fclose(boot_file);
	fclose(map_file);

	return boot;
}

int main(int argc, char **argv)
{
	const char *rom_file;
	struct boot_data *boot;
	WINDOW *menu_window;
	bool success;

	if (argc != 2) {
		fprintf(stderr, "Invocation: %s <rom-file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	rom_file = argv[1];

	boot = load_boot_data(rom_file);
	if (boot == NULL) {
		fprintf(stderr, "Failed to read boot data\n");
		return EXIT_FAILURE;
	}

	initscr();
	noecho();
	curs_set(false);

	menu_window = newwin(getmaxy(stdscr), getmaxx(stdscr), 0, 0);
	keypad(menu_window, true);

	run_main_menu(menu_window, boot, rom_file);

	delwin(menu_window);

	endwin();

	success = store_boot_data(boot,
				  rom_file,
				  "bootorder_out",
				  "bootorder_bin_out",
				  "bootorder_map_out");

	boot_data_free(boot);

	return (success ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* vim: set ts=8 sts=8 sw=8 noet : */
