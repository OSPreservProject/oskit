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
 * Convenience function to open a disk partition.
 */

#include <oskit/startup.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/io/blkio.h>
#include <oskit/diskpart/diskpart.h>
#include <stdio.h>
extern int oskit_usermode_simulation;

#define MAX_PARTS 30

/*
 * Open the named disk partition and return an oskit_blkio_t COM object.
 * `disk' names the disk device (e.g. "hda", "sda" with linux drivers).
 * If `partition' is null, returns the oskit_blkio_t object for the whole
 * disk; otherwise, it's a BSD slice/partition string like "s1" or "s3g"
 * or "a".
 */
oskit_error_t
start_disk(const char *disk, const char *partition, int read_only,
	   oskit_blkio_t **out_bio)
{
	int		numparts, err;
	oskit_blkio_t	*bio, *newbio;
	diskpart_t	*dp, part_array[MAX_PARTS];

	start_blk_devices();	/* make sure devices have been probed */

	/* Open the disk.  */
	err = oskit_linux_block_open(disk,
				     OSKIT_DEV_OPEN_READ|
				     (read_only ? 0 : OSKIT_DEV_OPEN_WRITE),
				     &bio);

	switch (err) {
	case 0:
		break;
	case OSKIT_E_DEV_NOSUCH_DEV:
		printf("start_disk: disk %s does not exist!\n", disk);
		return OSKIT_ENOENT;
	default:
		printf("start_disk: error %x opening disk %s\n", err, disk);
		return OSKIT_E_UNEXPECTED;
	}

	if (partition == 0) {
		/*
		 * No partition requested, just return the whole disk.
		 */
		*out_bio = bio;
		return 0;
	}

	/*
	 * In usermode simulation, operate on the entire disk only.
	 * No partitions!
	 */
	if (oskit_usermode_simulation) {
		*out_bio = bio;
		return 0;
	}

	/*
	 * Get the partition info.
	 */
	numparts = diskpart_blkio_get_partition(bio, part_array, MAX_PARTS);
	if (numparts == 0) {
		printf("start_disk: No partitions found\n");
		oskit_blkio_release(bio);
		return OSKIT_E_UNEXPECTED;
	}

	/*
	 * Narrow to the requested partition.
	 */
	dp = diskpart_blkio_lookup_bsd_string(part_array,
					      partition, bio, &newbio);

	if (dp == NULL) {
		printf("start_disk: No such partition: %s\n", partition);
		oskit_blkio_release(bio);
		return OSKIT_E_UNEXPECTED;
	}

	/*
	 * Don't need the disk anymore, the partition has a ref.
	 */
	oskit_blkio_release(bio);

	*out_bio = newbio;
	return 0;
}
