/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "utils.h"

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *format_str(const char format[], ...)
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

bool skip_prefix(const char **str, const char *prefix)
{
	const size_t prefix_len = strlen(prefix);
	if (strncmp(*str, prefix, prefix_len) == 0) {
		*str += prefix_len;
		return true;
	}
	return false;
}

FILE *temp_file(char *template)
{
	int fd;
	FILE *file;

	fd = mkstemp(template);
	if (fd == -1)
		return NULL;

	file = fdopen(fd, "r+");
	if (file == NULL) {
		(void)unlink(template);
		return NULL;
	}

	return file;
}
