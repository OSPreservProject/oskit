/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * An interface to diskpart_get_partition that takes a COM oskit_blkio_t.
 */

#include <oskit/types.h>
#include <oskit/io/blkio.h>
#include <oskit/diskpart/diskpart.h>
#include <oskit/c/stdio.h>
#include <oskit/c/string.h>

/*
 * Callback that reads into buf the block starting at start.
 * Returns zero on success.
 */
static int
blkio_read(void *arg, int start, char *buf)
{
	oskit_blkio_t *b = (oskit_blkio_t *) arg;
	oskit_size_t blksize;
	oskit_error_t err;
	oskit_size_t nbytes;

	blksize = oskit_blkio_getblocksize(b);
	err = oskit_blkio_read(b, buf, start * blksize, blksize, &nbytes);
	if (err)
		return err;

	return 0;
}

/*
 * This returns the number of partitions found.
 */
int
diskpart_blkio_get_partition(oskit_blkio_t *b, struct diskpart *array,
			     int array_size)
{
	oskit_off_t disk_bytes;
	int disk_sectors;
	oskit_size_t block_bytes;
	oskit_error_t err;

	err = oskit_blkio_getsize(b, &disk_bytes);
	if (err)
		return 0;
	block_bytes = oskit_blkio_getblocksize(b);

        /*
         * Linux drivers return a 0 length blkio if you open a
         * non-existent disk.  This seems to make everything really
         * unhappy.
         */
        if (disk_bytes == 0) {
                return 0;
        }
	/*
	 * We do this instead of the obvious disk_bytes/block_bytes because
	 * disk_bytes is a 64 bit number and GCC makes us use special
	 * functions from -lgcc to do 64 bit arith but we don't want to
	 * link in -lgcc.
	 *
	 * block_bytes is guaranteed to be a power of two,
	 * therefore ffs-1 gives the correct El Shiftage.
	 */
	disk_sectors = (disk_bytes >> (ffs(block_bytes) - 1));

	return diskpart_get_partition(b, blkio_read, array, array_size,
				      disk_sectors);
}
