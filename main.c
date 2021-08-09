#include <curses.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "boot_order.h"
#include "list_menu.h"
#include "options.h"
#include "utils.h"

static void make_boot_menu(struct list_menu *menu, struct boot_data *boot)
{
	int i;

	list_menu_clear(menu);

	for (i = 0; i < boot->record_count; ++i) {
		char *item = format_str("(%c)  %s",
					'a' + i,
					boot->records[i].name);
		list_menu_add_item(menu, item);
		free(item);
	}
}

void run_boot_menu(WINDOW *menu_window, struct boot_data *boot)
{
	struct list_menu *boot_menu;

	boot_menu = list_menu_new("coreboot configuration :: boot order");

	make_boot_menu(boot_menu, boot);

	while (true) {
		const int key = list_menu_run(boot_menu, menu_window);
		if (key == ERR || key == 'q')
			break;

		if (key >= 'a' && key < 'a' + boot->record_count) {
			const int item = key - 'a';
			const int line = boot_menu->current;

			if (item >= line)
				ROTATE_RIGHT(&boot->records[line],
					     item - line + 1);
			else
				ROTATE_LEFT(&boot->records[item],
					    line - item + 1);

			make_boot_menu(boot_menu, boot);
			list_menu_goto(boot_menu, line + 1);
		}
	}

	list_menu_free(boot_menu);
}

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
				run_boot_menu(menu_window, boot);
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
			    const char *boot_def_file_path,
			    const char *boot_file_path,
			    const char *map_file_path)
{
	FILE *file;
	bool success;

	file = fopen_wrapper(boot_def_file_path, "w");
	if (file == NULL)
		return false;
	boot_data_dump_boot(boot, file);
	if (!fclose_wrapper(file, boot_def_file_path))
		return false;

	file = fopen_wrapper(boot_file_path, "w");
	if (file == NULL)
		return false;
	boot_data_dump_boot(boot, file);
	success = pad_file(file);
	if (!fclose_wrapper(file, boot_file_path) || !success)
		return false;

	file = fopen_wrapper(map_file_path, "w");
	if (file == NULL)
		return false;
	boot_data_dump_map(boot, file);
	if (!fclose_wrapper(file, map_file_path))
		return false;

	return true;
}

static void run_from_fork(int pipe[2], char **argv)
{
	int null_fd;

	/* Close read end of the pipe. */
	(void)close(pipe[0]);

	/* Redirect output stream to write end of the pipe */
	if (dup2(pipe[1], STDERR_FILENO) == -1)
		_Exit(EXIT_FAILURE);
	if (dup2(pipe[1], STDOUT_FILENO) == -1)
		_Exit(EXIT_FAILURE);

	if (pipe[1] != STDERR_FILENO && pipe[1] != STDOUT_FILENO)
		/* Close write end of the pipe after it was duplicated */
		(void)close(pipe[1]);

	null_fd = open("/dev/null", O_RDWR);
	if (null_fd == -1)
		_Exit(EXIT_FAILURE);

	if (dup2(null_fd, STDIN_FILENO) == -1)
		_Exit(EXIT_FAILURE);

	if (null_fd != STDIN_FILENO && null_fd != STDOUT_FILENO)
		(void)close(null_fd);

	execvp(argv[0], argv);
	_Exit(127);
}

static int child_wait(pid_t pid)
{
	while (true) {
		int status;
		if (waitpid(pid, &status, 0) == -1) {
			if (errno == EINTR)
				continue;
			break;
		}

		if (WIFEXITED(status) || WIFSIGNALED(status))
			return status;
	}

	return -1;
}

static bool run_cmd(char **argv)
{
	FILE *file;
	pid_t pid;
	int out_pipe[2];

	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	int exit_code;

	if (pipe(out_pipe) != 0)
		return false;

	pid = fork();
	if (pid == (pid_t)-1)
		return false;

	if (pid == 0) {
		run_from_fork(out_pipe, argv);
		return false;
	}

	/* Close write end of pipe. */
	close(out_pipe[1]);

	file = fdopen(out_pipe[0], "r");
	if (file == NULL)
		close(out_pipe[0]);

	while ((read = getline(&line, &len, file)) != -1)
		printf("%s: %s", argv[0], line);
	free(line);

	fclose(file);

	exit_code = child_wait(pid);
	if (exit_code == -1) {
		fprintf(stderr, "Waiting for %s has failed", argv[0]);
		return false;
	}

	return (exit_code == EXIT_SUCCESS);
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
	if (file == NULL)
		return NULL;

	if (!run_cmd(argv)) {
		(void)fclose(file);
		fprintf(stderr,
			"Failed to extract %s from %s\n",
			name, rom_file);
		return NULL;
	}

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
				  "bootorder_out",
				  "bootorder_bin_out",
				  "bootorder_map_out");

	boot_data_free(boot);

	return (success ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* vim: set ts=8 sts=8 sw=8 noet : */
