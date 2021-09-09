/* SPDX-License-Identifier: BSD-3-Clause or GPL-2.0-only */

#ifndef FLASHMAP_LIB_FMAP_H__
#define FLASHMAP_LIB_FMAP_H__

#include <inttypes.h>

#define FMAP_SIGNATURE		"__FMAP__"
#define FMAP_VER_MAJOR		1	/* this header's FMAP minor version */
#define FMAP_VER_MINOR		1	/* this header's FMAP minor version */
#define FMAP_STRLEN		32	/* maximum length for strings, */
					/* including null-terminator */

/* Mapping of volatile and static regions in firmware binary */
struct fmap_area {
	uint32_t offset;		/* offset relative to base */
	uint32_t size;			/* size in bytes */
	uint8_t  name[FMAP_STRLEN];	/* descriptive name */
	uint16_t flags;			/* flags for this area */
} __attribute__((packed));

struct fmap {
	uint8_t  signature[8];		/* "__FMAP__" (0x5F5F464D41505F5F) */
	uint8_t  ver_major;		/* major version */
	uint8_t  ver_minor;		/* minor version */
	uint64_t base;			/* address of the firmware binary */
	uint32_t size;			/* size of firmware binary in bytes */
	uint8_t  name[FMAP_STRLEN];	/* name of this firmware binary */
	uint16_t nareas;		/* number of areas described by
					   fmap_areas[] below */
	struct fmap_area areas[];
} __attribute__((packed));

/*
 * fmap_find - find FMAP signature in a binary image
 *
 * @image:	binary image
 * @len:	length of binary image
 *
 * This function does no error checking. The caller is responsible for
 * verifying that the contents are sane.
 *
 * returns offset of FMAP signature to indicate success
 * returns <0 to indicate failure
 */
extern long int fmap_find(const uint8_t *image, unsigned int len);

/*
 * fmap_size - returns size of fmap data structure (including areas)
 *
 * @fmap:	fmap
 *
 * returns size of fmap structure if successful
 * returns <0 to indicate failure
 */
extern int fmap_size(const struct fmap *fmap);

/*
 * fmap_find_area - find an fmap_area entry (by name) and return pointer to it
 *
 * @fmap:	fmap structure to parse
 * @name:	name of area to find
 *
 * returns a pointer to the entry in the fmap structure if successful
 * returns NULL to indicate failure or if no matching area entry is found
 */
extern const struct fmap_area *fmap_find_area(const struct fmap *fmap,
							const char *name);

#endif	/* FLASHMAP_LIB_FMAP_H__*/
