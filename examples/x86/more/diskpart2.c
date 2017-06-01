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
#include <stdio.h>
#include <malloc.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <oskit/io/blkio.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/tty.h>
#include <oskit/debug.h>
#include <oskit/lmm.h>		
#include <oskit/c/termios.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/pc/com_cons.h>
#include <oskit/x86/pc/reset.h>
#include <oskit/x86/pc/direct_cons.h>
#include <oskit/x86/pc/phys_lmm.h>
#include <oskit/gdb.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

/* This is the testing program for the partitioning code. */
#include <oskit/diskpart/diskpart.h>

#define MAX_PARTS 30

diskpart_t part_array[MAX_PARTS];

/* This is bad: how do I find this? */
#define DISK_SIZE 10000

/*
 * I use the driver_info parameter (void *) to pass the NAME of
 * the device, as that is all I really need to know.
 * I could keep it open between requests, but then closing it
 * is more of an issue (as are multiple disks being read).
 */

int 
my_read_fun(void *driver_info, int start, char *buf)
{
        char *driver_name = (char*) driver_info;
	oskit_blkio_t *io = NULL;
	int err, amt;
	static char *newbuf=NULL;

	if (!newbuf) {
		printf("grabbing buffer!\n");
		newbuf = lmm_alloc_aligned(&malloc_lmm, 512, 0, 9, 0);
		if (!newbuf) {
			printf("couldn't allocate tmp buffer!\n");
			_exit(1);
		}
	}

	printf("Opening disk %s...", driver_name);
	err = oskit_linux_block_open(driver_name, OSKIT_DEV_OPEN_READ, &io);
	if (err) {
		if (err == OSKIT_E_DEV_NOSUCH_DEV)
			printf("disk %s does not exist!\n", driver_name);
		else	
			printf("error %x opening disk %s\n", err, 
			       driver_name);

		return -1;
	}

	printf("\nreading block %d, buf = %p\n", start, buf);

	err = oskit_blkio_read(io, newbuf, (512 * start), 512, &amt);
	if (err) {
		printf("  error %d reading at block %x\n", err, start);
		return -1;
	} 

	memcpy(buf, newbuf, 512);

	printf("done reading block %d\n", start);

	oskit_blkio_release(io);

        return(0);
}

int
main(int argc, char **argv)
{
	int numparts;
	char name[10];
	int found = 0;
	int more = 1;

	oskit_clientos_init();
	start_blk_devices();

	/* Okay, print out partition info for all desired disks */
	while (more) {
		printf("What disk do you want to check?"
		       " (eg, \"sd0\", \"quit\" to exit): ");
		gets(name);

		if (*name == '\0')
			continue;

		if (!strcmp(name, "quit"))
			break;

		printf("\nGetting partition info for %s\n", name);

		/* call the partition code */
        	numparts = diskpart_get_partition((void *)name, 
				  (*my_read_fun), part_array,
				  MAX_PARTS, DISK_SIZE);

		if (numparts > 0) {
			found++;
        		printf("%d partitions found\n", numparts);
        		diskpart_dump(part_array, 0, 'a');
		} else {
			printf("No partitions found\n");
		}
	}

	printf("\nPartitions found on %d disks\n", found);

	return 0;
}
