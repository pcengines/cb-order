#include "ui_options.h"

#include <curses.h>
#include <readline/readline.h>

#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "boot_data.h"
#include "ui_screen.h"
#include "utils.h"

static struct
{
	const char *title;
	const char *description;
	const char *initial;
	WINDOW *window;
}
prompt_data;

static char *format_option_item(struct option *option)
{
	const struct option_def *option_def = &OPTIONS[option->id];

	char *value = NULL;
	char *line = NULL;

	switch (option_def->type) {
		case OPT_TYPE_BOOLEAN:
			value = strdup(option->value ? "on" : "off");
			break;
		case OPT_TYPE_TOGGLE:
			value = strdup(option->value ? "first" : "second");
			break;
		case OPT_TYPE_HEX4:
			value = format_str("%d", option->value);
			break;
	}

	line = format_str("(%c)  [%-11s = %6s]  %s",
			  option_def->shortcut,
			  option_def->keyword,
			  value,
			  option_def->description);

	free(value);
	return line;
}

static void fill_options_screen(struct screen *screen, struct boot_data *boot)
{
	int i;

	screen_clear_items(screen);

	for (i = 0; i < boot->option_count; ++i) {
		char *item = format_option_item(&boot->options[i]);
		screen_add_item(screen, item);
		free(item);
	}
}

static void completion_display_matches_hook(char **matches, int start, int end)
{
	/* Do nothing. */
}

static void prompt_redisplay(void)
{
	WINDOW *window = prompt_data.window;

	werase(window);
	box(window, ACS_VLINE, ACS_HLINE);
	mvwprintw(window, 0, 2, " %s ", prompt_data.title);

	mvwprintw(window, 2, 2,
		  "%s%s", rl_display_prompt, rl_line_buffer);
	mvwprintw(window, 4, 2, "%s", prompt_data.description);

	wmove(window, 2, 2 + strlen(rl_display_prompt) + rl_point);
	wrefresh(window);
}

static int prompt_startup(void)
{
	const size_t initial_len = strlen(prompt_data.initial);

	rl_extend_line_buffer(initial_len + 1U);
	strcpy(rl_line_buffer, prompt_data.initial);

	rl_point = initial_len;
	rl_end = initial_len;

	return 0;
}

static int prompt_input_available(void)
{
	int c;
	int tty;
	int flags;

	tty = fileno(rl_instream);

	/* Make reading non-blocking */
	flags = fcntl(tty, F_GETFL, 0);
	fcntl(tty, F_SETFL, flags | O_NONBLOCK);

	/* Check whether there is more input */
	c = fgetc(rl_instream);
	if (c == EOF)
		clearerr(rl_instream);
	else
		ungetc(c, rl_instream);

	/* Make reading blocking again */
	fcntl(tty, F_SETFL, flags);

	return (c != EOF);
}

static char *get_input(WINDOW *window,
		       const char *title,
		       const char *description,
		       const char *prompt,
		       const char *initial)
{
	char *input;
	int visibility;

	prompt_data.title = title;
	prompt_data.description = description;
	prompt_data.initial = initial;
	prompt_data.window = window;

	rl_redisplay_function = &prompt_redisplay;
	/* Prevent displaying completion menu, which could mess up output */
	rl_completion_display_matches_hook = &completion_display_matches_hook;

	rl_startup_hook = &prompt_startup;

	rl_input_available_hook = &prompt_input_available;

	visibility = curs_set(1);
	input = readline(prompt);
	curs_set(visibility);

	return input;
}

static void toggle_option(struct option *option, WINDOW *window)
{
	char *title;
	char *input;
	const struct option_def *option_def = &OPTIONS[option->id];

	if (option_def->type != OPT_TYPE_HEX4) {
		option->value = !option->value;
		return;
	}

	if (option->value != 0) {
		option->value = 0;
		return;
	}

	title = format_str("coreboot configuration :: options :: %s",
			    option_def->description);
	input = get_input(window, title,
			  "Range: [0; 65535]", "New value: ", "");
	free(title);

	(void)boot_data_set_option(option, strtol(input, NULL, 10));
	free(input);
}

void options_run(WINDOW *window, struct boot_data *boot)
{
	struct screen *screen;

	screen = screen_new("coreboot configuration :: options");
	fill_options_screen(screen, boot);

	screen_add_hint(screen, "Down/j, Up/k             move cursor");
	screen_add_hint(screen, "Home/g, End              move cursor");
	screen_add_hint(screen, "Space/Enter/Right/l/(_)  toggle/set option");
	screen_add_hint(screen, "Backspace/Left/q/h       leave");

	while (true) {
		int i;

		const int key = screen_run(screen, window);
		if (key == ERR || key == 'q' || key == 'h' || key == KEY_LEFT ||
		    key == KEY_BACKSPACE || key == '\b')
			break;

		if (key == ' ' || key == '\n' || key == 'l' ||
		    key == KEY_RIGHT) {
			toggle_option(&boot->options[screen->current], window);
			fill_options_screen(screen, boot);
			continue;
		}

		for (i = 0; i < boot->option_count; ++i) {
			struct option *option = &boot->options[i];
			const int id = option->id;
			const struct option_def *option_def = &OPTIONS[id];

			if (option_def->shortcut == key) {
				toggle_option(option, window);
				fill_options_screen(screen, boot);
				screen_goto(screen, i);
				break;
			}
		}
	}

	screen_free(screen);
}

/* vim: set ts=8 sts=8 sw=8 noet : */
