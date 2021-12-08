/* read and write binary file "partitions" described by FMAP */
/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef PARITITONED_FILE_H_
#define PARITITONED_FILE_H_

#include "common.h"
#include "fmap.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct partitioned_file partitioned_file_t;

/**
 * Read a file back in from the disk.
 * An in-memory buffer is created and populated with the file's
 * contents. If the image contains an FMAP, it will be opened as a
 * full partitioned file; otherwise, it will be opened as a flat file as
 * if it had been created by partitioned_file_create_flat().
 * The partitioned_file_t returned from this function is separately owned by the
 * caller, and must later be passed to partitioned_file_close();
 *
 * @param filename      Name of the file to read in
 * @param write_access  True if the file needs to be modified
 * @return              Caller-owned partitioned file, or NULL on error
 */
partitioned_file_t *partitioned_file_reopen(const char *filename,
					    bool write_access);

/**
 * Write a buffer's contents to its original region within a segmented file.
 * This function should only be called on buffers originally retrieved by a call
 * to partitioned_file_read_region() on the same partitioned file object. The
 * contents of this buffer are copied back to the same region of the buffer and
 * backing file that the region occupied before.
 *
 * @param file   Partitioned file to which to write the data
 * @param buffer Modified buffer obtained from partitioned_file_read_region()
 * @return       Whether the operation was successful
 */
bool partitioned_file_write_region(partitioned_file_t *file,
						const struct buffer *buffer);

/**
 * Obtain one particular region of a segmented file.
 * The result is owned by the partitioned_file_t and shared among every caller
 * of this function. Thus, it is an error to buffer_delete() it; instead, clean
 * up the entire partitioned_file_t once it's no longer needed with a single
 * call to partitioned_file_close().
 * Note that, if the buffer obtained from this function is modified, the changes
 * will be reflected in any buffers handed out---whether earlier or later---for
 * any region inclusive of the altered location(s). However, the backing file
 * will not be updated until someone calls partitioned_file_write_region() on a
 * buffer that includes the alterations.
 *
 * @param dest   Empty destination buffer for the data
 * @param file   Partitioned file from which to read the data
 * @param region Name of the desired FMAP region
 * @return       Whether the copy was performed successfully
 */
bool partitioned_file_read_region(struct buffer *dest,
			const partitioned_file_t *file, const char *region);

/** @param file Partitioned file to flush and cleanup */
void partitioned_file_close(partitioned_file_t *file);

/** @return Whether to include area in the running count. */
typedef bool (*partitioned_file_fmap_selector_t)
				(const struct fmap_area *area, const void *arg);

/** Selector that counts FMAP sections that are descendants of fmap_area arg. */
extern const partitioned_file_fmap_selector_t
				partitioned_file_fmap_select_children_of;

#endif
