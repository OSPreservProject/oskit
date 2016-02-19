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
 * Example of using the fsread library.
 *
 * It opens a file "/etc/passwd" and dumps it out.
 *
 * You will need to make sure DISK_NAME, PART_NAME, and FILE_NAME are
 * appropriate.
 */

#if 0
# define DISK_NAME	"sd0"		/* shaky */
# define PART_NAME	"a"
#else
# define DISK_NAME	"wd1"		/* pencil */
# define PART_NAME	"a"
#endif

#define FILE_NAME	"/etc/passwd"

#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/io/blkio.h>
#include <oskit/fs/read.h>
#include <oskit/diskpart/diskpart.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifndef OSKIT_UNIX
/* Partition array, filled in by the diskpart library. */
#define MAX_PARTS 30
static diskpart_t part_array[MAX_PARTS];
#endif

#define CHECK(err, f, args) ({			\
	(err) = f args;			\
	if (err)				\
		panic(#f" failed: %s (0x%lx)", strerror(err), (err));\
})

int
main(int argc, char **argv)
{
	char buf[BUFSIZ];
	oskit_blkio_t *disk;
	oskit_blkio_t *part;
	oskit_blkio_t *fbio;
	oskit_u32_t actual;
	oskit_u32_t i;
	oskit_error_t err;
	oskit_off_t offset;
	char *diskname = DISK_NAME;
	char *partname = PART_NAME;
	char *filename = FILE_NAME;
#ifndef OSKIT_UNIX
	int nparts;
#endif

#ifndef KNIT
	oskit_clientos_init();
	start_blk_devices();
#endif

#ifdef OSKIT_UNIX
	if (argc < 2)
		panic("Usage: %s fsfile [file]\n", argv[0]);
	diskname = argv[1];
	partname = "not used";
	filename = (argc > 2) ? argv[2] : FILE_NAME;
#endif

	printf(">>>Opening the disk %s\n", diskname);
	CHECK(err, oskit_linux_block_open, (diskname, OSKIT_DEV_OPEN_READ, &disk));
#ifdef OSKIT_UNIX
	part = disk;
	oskit_blkio_addref(disk);
#else
	nparts = diskpart_blkio_get_partition(disk, part_array, MAX_PARTS);
	assert(nparts > 0);
	if (diskpart_blkio_lookup_bsd_string(part_array, partname,
					     disk, &part) == 0) {
		printf("Couldn't find partition %s\n", partname);
		exit(1);
	}
#endif /* OSKIT_UNIX */
	

	/* Don't need the disk anymore, the partition has a ref. */
	oskit_blkio_release(disk);

	CHECK(err, fsread_ffs_open, (part, filename, &fbio));
	oskit_blkio_release(part);	/* fbio has a ref */
	offset = 0;
	do {
		CHECK(err, oskit_blkio_read, (fbio, buf, offset, BUFSIZ, &actual));
		for (i = 0; i < actual; i++)
			putchar(buf[i]);
		offset += actual;
	} while (actual);

	printf(">>>Releasing fbio\n");
	assert(oskit_blkio_release(fbio) == 0);

	return 0;
}

