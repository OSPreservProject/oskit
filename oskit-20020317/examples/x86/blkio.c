/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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
 * This is an example OS-Kit program that tests the disk code in the
 * block device drivers.  It reads blocks of data from the disk.
 * You can change REALLY_WRITE to control writing to the disk.  If there
 * is a problem, the contents of the disk may no longer be valid.
 * However, *IF* everything works correctly, this program will restore
 * the previous contents of the disk when it is done.
 */

#include <stdio.h>
#include <malloc.h>
#include <oskit/io/blkio.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/tty.h>
#include <oskit/debug.h>
#include <oskit/c/termios.h>
#include <oskit/c/string.h>
#include <oskit/machine/base_vm.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <oskit/c/unistd.h>	/* _exit */

oskit_blkio_t *io;

/* #define REALLY_WRITE if you want to actually write. */

/* These are in terms of SECTORS. */
int test_offset[] = { 0, 1, 3,  7, 8, 63, 64, 127, 128, 1024, 1025 };
int test_size[]   = { 1, 2, 5, 11, 8, 63, 64, 128, 129, 333, 1024, 1025 };

#define n_offsets (sizeof(test_offset) / sizeof(test_offset[0]))
#define n_sizes (sizeof(test_size) / sizeof(test_size[0]))

int
read_test(int offset, int size, char *buf)
{
	int err, amt;

#ifdef DEBUG_TEST
	printf("Reading %d bytes at offset %d\n", size,
		offset);
#endif

	err = oskit_blkio_read(io, buf, offset, size, &amt);
	if (err) {
		printf("  Read ERROR %08x\n", err);
		return(1);
	}

#ifdef DEBUG_TEST
	printf("Read done, amt = %d of %d\n", amt, size);
#endif

	return 0;
}

#ifdef REALLY_WRITE
int
write_test(int offset, int size, char *buf)
{
	int err, amt;

#ifdef DEBUG_TEST
	printf("Writing %d bytes at offset %d\n", size,
		offset);
#endif

	err = oskit_blkio_write(io, buf, offset, size, &amt);
	if (err || amt != size) {
		printf("  Write ERROR %d\n", err);
                return(1);
        }
#ifdef DEBUG_TEST
	printf("Wrote %d of %d bytes!\n", amt, size);
#endif

	return 0;
}
#endif /* REALLY_WRITE */


/*
 * This runs tests on a bunch of disk blocks.
 * Generally, these should be an integral multiple of sectors aligned
 * on a sector boundary.
 */
int
test_block(int offset, int size)
{
	char *wbuf;
#ifdef REALLY_WRITE
	char *vbuf;
	int i;
#endif

	/* Allocate memory for the data */
	wbuf = smemalign(512, size);
	if (!wbuf) {
		printf("  can't allocate buf!\n");
		_exit(1);
	}
	memset(wbuf, 0xA5, size);

#ifdef REALLY_WRITE
	vbuf = smemalign(512, size);
	if (!vbuf) {
		printf("  can't allocate buf!\n");
		_exit(1);
	}
	memset(vbuf, 0xA5, size);
#endif

#ifdef REALLY_WRITE
	printf("Doing read  test of %d bytes at offset %d\n", size, offset);
#else
	printf("Doing read test of %d bytes at offset %d\n", size, offset);
#endif

	/* Read in blocks from the disk. */
	if (read_test(offset, size, wbuf))
		return -1;

#ifdef DEBUG_TEST
	printf("read %x\n", wbuf[0] & 0xff);
#endif

#ifdef REALLY_WRITE
	/*
	 * Okay, here we change the data (invert the bits), and
	 * write it back out.
	 */
	for (i = 0; i < size; i++)
		wbuf[i] = ~wbuf[i];

	printf("Doing write test of %d bytes at offset %d\n", size, offset);

	if (write_test(offset, size, wbuf))
		return -1;

#ifdef DEBUG_TEST
	printf("inverted bits\n");
	printf("wrote %x\n", wbuf[0] & 0xff);
#endif

	/* After we write it out, we read it back, and check it. */
	if (read_test(offset, size, vbuf)) {
		printf("Error reading back data!\n");
		return -1;
	}

#ifdef DEBUG_TEST
	printf("read %x\n", vbuf[0] & 0xff);
#endif

	/* now, make sure they are all the same... */
	for (i = 0; i < size; i++)
		if (wbuf[i] != vbuf[i]) {
			printf("ERROR: differing at byte %d (%x != %x)\n", i,
			       wbuf[i], vbuf[i]);
			break;	/* to avoid TONS of errors... */
		}

	/* Restore the data, and re-write it out. */
	for (i = 0; i < size; i++)
		wbuf[i] = ~wbuf[i];

	if (write_test(offset, size, wbuf))
		return -1;
#ifdef DEBUG_TEST
	printf("inverted bits\n");
	printf("wrote %x\n", wbuf[0] & 0xff);
#endif

	/* release the write-test memory */
	sfree(vbuf, size);
#endif /* REALLY_WRITE */

	/* release the memory */
	sfree(wbuf, size);

	return 0;
}


int
main(int argc, char **argv)
{
	int rc;
	int err = 0;
	char name[10];
	oskit_size_t sec_size;
	oskit_off_t disk_size;
	int n_tests, i, j;

#ifndef KNIT
	oskit_clientos_init();
	start_blk_devices();
#endif
	oskit_dump_devices();
#ifdef  GPROF
	start_fs_bmod();
	start_gprof();
	pause_gprof(0);
#endif

#ifdef REALLY_WRITE
	printf("\nWARNING: Doing Read/Write test.\n");
#else
	printf("\nDoing Read-Only test.\n");
#endif

	/*
	 * Get the drive name to test from the user.
	 * We don't want to trash the wrong disk ...
	 */
	do {
		printf("\nEnter drive name to test, `quit' to exit: ");
		gets(name);

		if (!strcmp(name, "quit"))
			return 1;

		if (!strcmp(name, ""))
			continue;

		printf("Opening disk %s...\n", name);
		err = oskit_linux_block_open(name, OSKIT_DEV_OPEN_READ
#ifdef REALLY_WRITE
			| OSKIT_DEV_OPEN_WRITE
#endif
			, &io);

		if (err) {
			if (err == OSKIT_E_DEV_NOSUCH_DEV)
				printf("disk %s does not exist!\n", name);
			else	
				printf("error %x opening disk %s\n", 
				       err, name);
		}
		
	} while (err);

	sec_size = oskit_blkio_getblocksize(io);
	printf("disk native sectors are %d bytes\n", sec_size);

	if (oskit_blkio_getsize(io, &disk_size)) {
		printf("Can't determine disk size...assuming the disk is big enough!\n");
		disk_size = (test_offset[n_offsets - 1] + 
			    test_size[n_sizes - 1]) * sec_size;
	} else
		printf("size of disk is %d kbytes\n", 
		       (unsigned int)(disk_size >> 10));

	n_tests = n_offsets;
	printf("\nRunning %d tests\n\n", n_tests);
	
#ifdef  GPROF
	pause_gprof(1);
#endif
	for (i = 0; i < n_tests; i++) {
		for (j = 0; j < n_sizes; j++) {
			if (((test_offset[i] + test_size[j]) * sec_size) <=
			    disk_size) {
				rc = test_block(test_offset[i] * sec_size, 
						test_size[j] * sec_size);

				if (rc) {
					printf("\nEXITING: errors in the disk tests.\n");
				
					oskit_blkio_release(io);
					return rc;
				}
			}
		}
	}
#ifdef  GPROF
	pause_gprof(0);
#endif
	oskit_blkio_release(io);

	printf("\nTests completed with no errors.\n");

	return 0;
}

