/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef UTILS_H__
#define UTILS_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))

#define GROW_ARRAY(array, size) \
	({ \
		void *ptr = realloc((array), sizeof(*array)*((size) + 1)); \
		if (ptr != NULL) { \
			(array) = ptr; \
		} \
		(ptr == NULL ? NULL : &(array)[size]); \
	})

/* Cyclically rotates (slice of) an array one element to the right */
#define ROTATE_RIGHT(array, size) \
	do { \
		char buf[sizeof(*(array))]; \
		if ((size) <= 0) \
			break; \
		memcpy(buf, &(array)[(size) - 1], sizeof(buf)); \
		memmove((array) + 1, (array), sizeof(*(array))*((size) - 1)); \
		memcpy((array), buf, sizeof(buf)); \
	} while (false)

/* Cyclically rotates (slice of) an array one element to the left */
#define ROTATE_LEFT(array, size) \
	do { \
		char buf[sizeof(*(array))]; \
		if ((size) <= 0) \
			break; \
		memcpy(buf, (array), sizeof(buf)); \
		memmove((array), (array) + 1, sizeof(*(array))*((size) - 1)); \
		memcpy(&(array)[(size) - 1], buf, sizeof(buf)); \
	} while (false)

char *format_str(const char format[], ...)
	__attribute__ ((format(printf, 1, 2)));

bool skip_prefix(const char **str, const char *prefix);

FILE *temp_file(char *template);

#endif // UTILS_H__
