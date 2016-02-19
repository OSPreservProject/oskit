/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * An alternative interface that takes a oskit_blkio_t and returns another.
 * The returned oskit_blkio_t is a blkio-subset and operations on it
 * are restricted to the bounds of the partition.
 * It also returns the struct diskpart, or NULL on error.
 */

#include <oskit/types.h>
#include <oskit/io/blkio.h>
#include <oskit/diskpart/diskpart.h>
#include <oskit/c/stdio.h>

struct diskpart *
diskpart_blkio_lookup_bsd_string(struct diskpart *array,
				 const char *name,
				 oskit_blkio_t *b,
				 oskit_blkio_t **out_b)
{
	struct diskpart *dp;
	oskit_error_t err;
	oskit_size_t blocksize;

	dp = diskpart_lookup_bsd_string(array, name);
	if (dp == NULL)
		return NULL;

	blocksize = oskit_blkio_getblocksize(b);
	err = oskit_blkio_subset(b, (oskit_off_t)dp->start * blocksize,
				 (oskit_off_t)dp->size * blocksize, out_b);
	if (err) {
		printf("%s: oskit_blkio_subset returned 0x%x\n",
		       __FUNCTION__, err);
		return NULL;
	}

	return dp;
}
