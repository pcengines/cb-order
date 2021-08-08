#include "utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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
