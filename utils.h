#ifndef UTILS_H__
#define UTILS_H__

#include <stdlib.h>

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))

#define GROW_ARRAY(array, size) \
	({ \
		void *ptr = realloc((array), sizeof(*array)*((size) + 1)); \
		if (ptr != NULL) { \
			(array) = ptr; \
		} \
		(ptr == NULL ? NULL : &(array)[size]); \
	})

char *format_str(const char format[], ...)
	__attribute__ ((format(gnu_printf, 1, 2)));

#endif // UTILS_H__

/* vim: set ts=8 sts=8 sw=8 noet : */
