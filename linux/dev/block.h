/*
 * Copyright (c) 1996, 1997, 1998 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Linux block emulation definitions.
 */
#ifndef _LINUX_BLOCK_H_
#define _LINUX_BLOCK_H_

#include <oskit/io/blkio.h>
#include <oskit/dev/blk.h>

/*
 * One of these structures exists for each
 * open block device.
 */
struct peropen {
	oskit_blkio_t ioi;		/* COM block I/O interface */
	unsigned count;			/* reference count */
	unsigned short busy:1;		/* someone is opening the device */
	unsigned short want:1;		/* someone wants the device */
	struct wait_queue *waitq;	/* for notifiations on this struct */
	int	mode;			/* open mode */
	struct	inode inode;		/* Linux inode structure */
	struct	file_operations *fops;	/* Linux file operations vector */
	int	bsize;			/* block size */
	int	bshift;			/* block shift */
	int	bmask;			/* block mask */
	unsigned size;			/* size in sectors (512 bytes) */
	struct	peropen *next;		/* next peropen in list */
};

/*
 * One these exists for each registered Linux block device driver.
 * Each of these structures represents a COM block device object,
 * whose primary interface is the oskit_blkdev interface, blki.
 *
 * Note that for Linux block devices that actually represent buses
 * whose individual devices are selected by minor number (e.g., IDE, SCSI),
 * the device node registered here is really the bus device node
 * (e.g., IDE controller or SCSI host adaptor);
 * the device nodes for the individual disks or whatever hang off of them.
 */
struct device_struct {

	/* The following members are set by oskit_linux_block_register() */
	oskit_blkdev_t blki;		/* COM fdev block device interface */
	oskit_devinfo_t *info;		/* Descriptive info on device */
	oskit_driver_t *drv;		/* Reference to driver interface */

	/* The following members are set by blkdev_register() */
	struct file_operations *fops;	/* Linux file operations vector */
	const char *name;		/* Device name */
};

extern struct device_struct blkdevs[];


#define DISK_BLOCK_SIZE		512
#define DISK_BLOCK_BITS 	9


/*
 * Register a Linux block device in the oskit device hierarchy,
 * as an immediate child of the standard ISA bus at the specified I/O address.
 */
oskit_error_t oskit_linux_block_register(int major, unsigned ioaddr,
					oskit_devinfo_t *info, oskit_driver_t *drv);

#endif /* _LINUX_BLOCK_H_ */
