#include "options.h"

#include <curses.h>

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "boot_order.h"
#include "list_menu.h"
#include "utils.h"

static char *format_str(const char format[], ...) __attribute__ ((format(gnu_printf, 1, 2)));
static char *format_str(const char format[], ...)
{
	va_list ap;
	va_list aq;
	size_t len;
	char *buf;

	va_start(ap, format);
	va_copy(aq, ap);

	len = vsnprintf(NULL, 0, format, ap);
	va_end(ap);

	buf = malloc(len + 1);
	if (buf != NULL)
		(void)vsprintf(buf, format, aq);
	va_end(aq);

	return buf;
}

static char *format_option_value(struct option *option)
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

	line = format_str("(%c)  [%6s]  %s",
			  option_def->shortcut,
			  value,
			  option_def->description);

	free(value);
	return line;
}

static void make_options_menu(struct list_menu *menu, struct boot_data *boot)
{
	int i;

	list_menu_clear(menu);

	for(i = 0; i < boot->option_count; ++i) {
		char *item = format_option_value(&boot->options[i]);
		list_menu_add_item(menu, item);
		free(item);
	}
}

static void toggle_option(struct option *option)
{
	const struct option_def *option_def = &OPTIONS[option->id];

	if (option_def->type != OPT_TYPE_HEX4) {
		option->value = !option->value;
		return;
	}

	if (option->value != 0) {
		option->value = 0;
		return;
	}

	// TODO: prompt user for a new value
}

void options_menu_run(WINDOW *menu_window, struct boot_data *boot)
{
	struct list_menu *options_menu;

	options_menu = list_menu_new("coreboot configuration :: options");
	make_options_menu(options_menu, boot);

	while (true) {
		int i;

		const int key = list_menu_run(options_menu, menu_window);
		if (key == 'q')
			break;

		if (key == ' ') {
			toggle_option(&boot->options[options_menu->current]);
			make_options_menu(options_menu, boot);
			continue;
		}

		for (i = 0; i < boot->option_count; ++i) {
			struct option *option = &boot->options[i];
			const enum option_id id = option->id;
			const struct option_def *option_def = &OPTIONS[id];

			if (option_def->shortcut == key) {
				toggle_option(option);
				make_options_menu(options_menu, boot);
				list_menu_goto(options_menu, i);
				break;
			}
		}
	}

	list_menu_free(options_menu);
}
