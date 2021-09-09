/* SPDX-License-Identifier: BSD-3-Clause or GPL-2.0-only */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>
#include <endian.h>
/* #include <commonlib/bsd/sysincludes.h> */

#include "fmap.h"

/* Make a best-effort assessment if the given fmap is real */
static int is_valid_fmap(const struct fmap *fmap)
{
	if (memcmp(fmap, FMAP_SIGNATURE, strlen(FMAP_SIGNATURE)) != 0)
		return 0;
	/* strings containing the magic tend to fail here */
	if (fmap->ver_major != FMAP_VER_MAJOR)
		return 0;
	/* a basic consistency check: flash should be larger than fmap */
	if (le32toh(fmap->size) <
		sizeof(*fmap) + le16toh(fmap->nareas) * sizeof(struct fmap_area))
		return 0;

	/* fmap-alikes along binary data tend to fail on having a valid,
	 * null-terminated string in the name field.*/
	int i = 0;
	while (i < FMAP_STRLEN) {
		if (fmap->name[i] == 0)
			break;
		if (!isgraph(fmap->name[i]))
			return 0;
		if (i == FMAP_STRLEN - 1) {
			/* name is specified to be null terminated single-word string
			 * without spaces. We did not break in the 0 test, we know it
			 * is a printable spaceless string but we're seeing FMAP_STRLEN
			 * symbols, which is one too many.
			 */
			 return 0;
		}
		i++;
	}
	return 1;

}

/* returns size of fmap data structure if successful, <0 to indicate error */
int fmap_size(const struct fmap *fmap)
{
	if (!fmap)
		return -1;

	return sizeof(*fmap) + (le16toh(fmap->nareas) * sizeof(struct fmap_area));
}

/* brute force linear search */
static long int fmap_lsearch(const uint8_t *image, size_t len)
{
	unsigned long int offset;
	int fmap_found = 0;

	for (offset = 0; offset < len - strlen(FMAP_SIGNATURE); offset++) {
		if (is_valid_fmap((const struct fmap *)&image[offset])) {
			fmap_found = 1;
			break;
		}
	}

	if (!fmap_found)
		return -1;

	if (offset + fmap_size((const struct fmap *)&image[offset]) > len)
		return -1;

	return offset;
}

/* if image length is a power of 2, use binary search */
static long int fmap_bsearch(const uint8_t *image, size_t len)
{
	unsigned long int offset = -1;
	int fmap_found = 0, stride;

	/*
	 * For efficient operation, we start with the largest stride possible
	 * and then decrease the stride on each iteration. Also, check for a
	 * remainder when modding the offset with the previous stride. This
	 * makes it so that each offset is only checked once.
	 */
	for (stride = len / 2; stride >= 16; stride /= 2) {
		if (fmap_found)
			break;

		for (offset = 0;
		     offset < len - strlen(FMAP_SIGNATURE);
		     offset += stride) {
			if ((offset % (stride * 2) == 0) && (offset != 0))
					continue;
			if (is_valid_fmap(
				(const struct fmap *)&image[offset])) {
				fmap_found = 1;
				break;
			}
		}
	}

	if (!fmap_found)
		return -1;

	if (offset + fmap_size((const struct fmap *)&image[offset]) > len)
		return -1;

	return offset;
}

static int popcnt(unsigned int u)
{
	int count;

	/* K&R method */
	for (count = 0; u; count++)
		u &= (u - 1);

	return count;
}

long int fmap_find(const uint8_t *image, unsigned int image_len)
{
	long int ret = -1;

	if ((image == NULL) || (image_len == 0))
		return -1;

	if (popcnt(image_len) == 1)
		ret = fmap_bsearch(image, image_len);
	else
		ret = fmap_lsearch(image, image_len);

	return ret;
}

const struct fmap_area *fmap_find_area(const struct fmap *fmap,
							const char *name)
{
	int i;
	const struct fmap_area *area = NULL;

	if (!fmap || !name)
		return NULL;

	for (i = 0; i < le16toh(fmap->nareas); i++) {
		if (!strcmp((const char *)fmap->areas[i].name, name)) {
			area = &fmap->areas[i];
			break;
		}
	}

	return area;
}
