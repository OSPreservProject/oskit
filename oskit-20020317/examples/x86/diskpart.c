/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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
 * This is an example of how to use the disk partition reading library.
 * This program prompts for a disk name and then prints out its partition
 * table (if the disk exists).
 */

#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/io/blkio.h>
#include <oskit/diskpart/diskpart.h>
#include <oskit/c/stdio.h>
#include <oskit/c/string.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

#define MAX_PARTS 30
diskpart_t part_array[MAX_PARTS];

int
main(int argc, char **argv)
{
	oskit_blkio_t *b;
	oskit_error_t err;
	int numparts;
	char name[10];
#ifndef KNIT
	oskit_clientos_init();
	start_blk_devices();
#endif
	for (;;) {
		printf("What disk do you want to check?"
		       " (eg, \"sd0\", \"quit\" to exit): ");
		gets(name);
		if (*name == '\0')
			continue;
		if (strcmp(name, "quit") == 0)
			break;

		printf("\nGetting partition info for %s\n", name);

		/* Open the disk. */
		err = oskit_linux_block_open(name, OSKIT_DEV_OPEN_READ, &b);
		switch (err) {
		case 0:
			break;
		case OSKIT_E_DEV_NOSUCH_DEV:
			printf("disk %s does not exist!\n", name);
			continue;
		default:
			printf("error %x opening disk %s\n", err, name);
			continue;
		}

		/*
		 * Get the partition info and dump it out.
		 * Another way to do this is by using diskpart_get_partition,
		 * which takes a callback function.
		 * That is, in fact, what diskpart_blkio_get_partition ends
		 * up calling.
		 */
		numparts = diskpart_blkio_get_partition(b, part_array,
							MAX_PARTS);
		if (numparts == 0) {
			printf("No partitions found\n");
			continue;
		}
		printf("%d partitions found\n", numparts);
		diskpart_dump(part_array, 0, 'a');

		/* Close the disk. */
		oskit_blkio_release(b);
	}

	return 0;
}
