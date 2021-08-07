#ifndef UTILS_H__
#define UTILS_H__

#include <stdlib.h>

#define GROW_ARRAY(array, size) \
	({ \
		void *ptr = realloc((array), sizeof(*array)*((size) + 1)); \
		if (ptr != NULL) { \
			(array) = ptr; \
		} \
		(ptr == NULL ? NULL : &(array)[size]); \
	})

#endif // UTILS_H__

/* vim: set ts=8 sts=8 sw=8 noet : */
