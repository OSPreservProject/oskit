/*
 * Copyright (c) 1996-1999 The University of Utah and the Flux Group.
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
 * Implement the device interface for Linux block drivers.
 *
 * XXX: The read/write routines presently assume a linear buffer
 * and do not make use of the buffer operations vectors.
 */

#include <oskit/dev/blk.h>
#include <oskit/dev/isa.h>
#include <oskit/dev/native.h>
#include <oskit/dev/linux.h>

#include <linux/fs.h>
#include <linux/blk.h>
#include <linux/fcntl.h>
#include <linux/major.h>
#include <linux/kdev_t.h>
#include <linux/string.h> /* memcmp */

#include "glue.h"
#include "block.h"
#include "linux_emul.h"
#include "sched.h"
#include "shared.h"
#include "osenv.h"

/*
 * Name to Linux device number mapping table.
 * XXX All this stuff should be separated out and moved to libfdev.
 */
static struct {
	char	*name;
	kdev_t	dev;
} name_to_dev[] = {
	{ "sd0", MKDEV(SCSI_DISK0_MAJOR, 0) },
	{ "sd1", MKDEV(SCSI_DISK0_MAJOR, 1) },
	{ "sd2", MKDEV(SCSI_DISK0_MAJOR, 2) },
	{ "sd3", MKDEV(SCSI_DISK0_MAJOR, 3) },
	{ "sd4", MKDEV(SCSI_DISK0_MAJOR, 4) },
	{ "sd5", MKDEV(SCSI_DISK0_MAJOR, 5) },
	{ "sd6", MKDEV(SCSI_DISK0_MAJOR, 6) },
	{ "sd7", MKDEV(SCSI_DISK0_MAJOR, 7) },
	{ "sda", MKDEV(SCSI_DISK0_MAJOR, 0) },
	{ "sdb", MKDEV(SCSI_DISK0_MAJOR, 1) },
	{ "sdc", MKDEV(SCSI_DISK0_MAJOR, 2) },
	{ "sdd", MKDEV(SCSI_DISK0_MAJOR, 3) },
	{ "sde", MKDEV(SCSI_DISK0_MAJOR, 4) },
	{ "sdf", MKDEV(SCSI_DISK0_MAJOR, 5) },
	{ "sdg", MKDEV(SCSI_DISK0_MAJOR, 6) },
	{ "sdh", MKDEV(SCSI_DISK0_MAJOR, 7) },
	{ "wd0", MKDEV(IDE0_MAJOR, 0) },
	{ "wd1", MKDEV(IDE0_MAJOR, 1) },
	{ "wd2", MKDEV(IDE1_MAJOR, 0) },
	{ "wd3", MKDEV(IDE1_MAJOR, 1) },
	{ "hda", MKDEV(IDE0_MAJOR, 0) },
	{ "hdb", MKDEV(IDE0_MAJOR, 1) },
	{ "hdc", MKDEV(IDE1_MAJOR, 0) },
	{ "hdd", MKDEV(IDE1_MAJOR, 1) },
};
#define NAME_TABLE_SIZE	(sizeof(name_to_dev) / sizeof(name_to_dev[0]))

/*
 * List of open devices.
 */
static struct peropen *open_list;

/*
 * Forward declarations.
 */
static OSKIT_COMDECL blkio_query(oskit_blkio_t *io, const struct oskit_guid *iid,
				void **out_ihandle);
static OSKIT_COMDECL_U blkio_addref(oskit_blkio_t *io);
static OSKIT_COMDECL_U blkio_release(oskit_blkio_t *io);
static OSKIT_COMDECL_U blkio_getblocksize(oskit_blkio_t *io);
static OSKIT_COMDECL blkio_read(oskit_blkio_t *io, void *buf,
			       oskit_off_t offset, oskit_size_t amount,
			       oskit_size_t *out_actual);
static OSKIT_COMDECL blkio_write(oskit_blkio_t *io, const void *buf,
				oskit_off_t offset, oskit_size_t amount,
				oskit_size_t *out_actual);
static OSKIT_COMDECL blkio_getsize(oskit_blkio_t *io, oskit_off_t *out_size);
static OSKIT_COMDECL blkio_setsize(oskit_blkio_t *io, oskit_off_t new_size);

static OSKIT_COMDECL bdev_query(oskit_blkdev_t *dev, const struct oskit_guid *iid,
			       void **out_ihandle);
static OSKIT_COMDECL_U bdev_addref(oskit_blkdev_t *dev);
static OSKIT_COMDECL_U bdev_release(oskit_blkdev_t *dev);
static OSKIT_COMDECL bdev_getinfo(oskit_blkdev_t *fdev, oskit_devinfo_t *out_info);
static OSKIT_COMDECL bdev_getdriver(oskit_blkdev_t *fdev,
				   oskit_driver_t **out_driver);
static OSKIT_COMDECL bdev_open(oskit_blkdev_t *dev, unsigned mode,
			      oskit_blkio_t **out_blkio);

/*
 * Driver operations vector for the per-open block I/O interface.
 */
static struct oskit_blkio_ops block_io_ops = {
	blkio_query,
	blkio_addref,
	blkio_release,
	blkio_getblocksize,
	blkio_read,
	blkio_write,
	blkio_getsize,
	blkio_setsize
};

/*
 * Driver operations vector for the block device interface.
 */
static struct oskit_blkdev_ops blk_dev_ops = {
	bdev_query,
	bdev_addref,
	bdev_release,
	bdev_getinfo,
	bdev_getdriver,
	bdev_open
};

/*
 * Register a Linux block device in the fdev device tree.
 */
oskit_error_t oskit_linux_block_register(int major, unsigned ioaddr,
					oskit_devinfo_t *info, oskit_driver_t *drv)
{
	/* Initialize the block device interface operations vector */
	blkdevs[major].blki.ops = &blk_dev_ops;
	blkdevs[major].info = info;
	blkdevs[major].drv = drv;
	oskit_driver_addref(drv);

	/* Add this block device node to the ISA bus */
	return osenv_isabus_addchild(ioaddr,
				 (oskit_device_t*)&blkdevs[major].blki);
}

/*
 * Remove a device from the open list.
 */
static inline void
open_list_remove(struct peropen *po)
{
	struct peropen *o, **pp;

	for (o = open_list, pp = &open_list; o; pp = &o->next, o = o->next) {
		if (o == po) {
			*pp = po->next;
			return;
		}
	}
}

/*
 * Open a Linux block device given its Linux name.
 */
oskit_error_t
oskit_linux_block_open(const char *name, unsigned flags, oskit_blkio_t **out_io)
{
	int i, len, part;
	kdev_t kdev;
	struct gendisk *gd;

	/*
	 * Convert name to device number.
	 */
	for (i = 0; i < NAME_TABLE_SIZE; i++) {
		len = strlen(name_to_dev[i].name);
		if (!strncmp(name_to_dev[i].name, name, len))
			break;
	}

	if (i == NAME_TABLE_SIZE
	    || !blkdevs[MAJOR(name_to_dev[i].dev)].fops)
		return (OSKIT_E_DEV_NOSUCH_DEV);

	kdev = name_to_dev[i].dev;

	/*
	 * Linux partition support.
	 *
	 * NOTE: This is only here for testing.
	 * It is NOT for general use; use the partition library.
	 */
	i = strlen(name);
	if (i - len > 1)
		return (OSKIT_E_DEV_NOSUCH_DEV);

	if (i - len == 1) {
		if (*(name + len) >= 'a' && *(name + len) <= 'f')
			part = *(name + len) - 'a' + 1;
		else if (*(name + len) >= '0' && *(name + len) <= '9')
			part = *(name + len) - '0';
		else
			return (OSKIT_E_DEV_NOSUCH_DEV);
	} else
		part = 0;

	/*
	 * Construct device number.
	 */

	for (gd = gendisk_head; gd; gd = gd->next) {
		if (gd->major == MAJOR(kdev)) {
			if (part >= gd->max_p)
				return (OSKIT_E_DEV_NOSUCH_DEV);

			kdev = MKDEV(MAJOR(kdev),
				     MINOR(kdev) << gd->minor_shift);
			kdev |= part;
			break;
		}
	}

	return oskit_linux_block_open_kdev(kdev, gd, flags, out_io);
}

/*
 * Open a Linux block device given its Linux device number.
 * The gendisk parameter need only be non-NULL if opening a partition;
 * it can be NULL when opening the entire disk ("partition 0").
 */
oskit_error_t
oskit_linux_block_open_kdev(int kdev, struct gendisk *gd,
		     unsigned flags, oskit_blkio_t **out_io)
{
	int err, i, mode;
	struct file file;
	struct peropen *po;
	struct task_struct ts;

	OSKIT_LINUX_CREATE_CURRENT(ts);

	if (flags & ~OSKIT_DEV_OPEN_ALL) {
		err = OSKIT_E_DEV_BADPARAM;
		goto finish;
	}

	switch (flags & (OSKIT_DEV_OPEN_READ|OSKIT_DEV_OPEN_WRITE)) {

	case OSKIT_DEV_OPEN_READ:
		mode = O_RDONLY;
		break;

	case OSKIT_DEV_OPEN_WRITE:
		mode = O_WRONLY;
		break;

	case OSKIT_DEV_OPEN_READ|OSKIT_DEV_OPEN_WRITE:
		mode = O_RDWR;
		break;

	default:
		err = OSKIT_E_DEV_BADPARAM;
		goto finish;
	}

	/*
	 * Linux uses 1k blocks by default.
	 * We WANT sector-sized blocks in all cases.
	 */
	if (!hardsect_size[MAJOR(kdev)]) {
		osenv_log(OSENV_LOG_DEBUG,
			"default sector size = 512b on %d,%d\n",
			MAJOR(kdev), MINOR(kdev));
		blksize_size[MAJOR(kdev)][MINOR(kdev)] = 512;

	} else if (blksize_size[MAJOR(kdev)][MINOR(kdev)]!=
	    hardsect_size[MAJOR(kdev)][MINOR(kdev)]) {
		osenv_log(OSENV_LOG_DEBUG,
			"setting block size to sector size on %d,%d\n",
			MAJOR(kdev), MINOR(kdev));
		blksize_size[MAJOR(kdev)][MINOR(kdev)] =
		    hardsect_size[MAJOR(kdev)][MINOR(kdev)];

		/* this is a Linux limitation: */
		if (hardsect_size[MAJOR(kdev)][MINOR(kdev)] < 512) {
			err = OSKIT_E_DEV_BADOP;	/* XXX */
			goto finish;
		}
	}

	/*
	 * Search the open list.
	 */
	while (1) {
		for (po = open_list; po; po = po->next)
			if (po->inode.i_rdev == kdev && po->mode == mode)
				break;
		if (po) {
			if (po->busy) {
				po->want = 1;
				sleep_on(&po->waitq);
				continue;
			}
			po->count++;
			*out_io = &po->ioi;
			err = 0;
			goto finish;
		}
		break;
	}

	/*
	 * Allocate and initialize a device structure.
	 */
	po = oskit_linux_mem_alloc(sizeof(struct peropen),
				  OSENV_PHYS_WIRED, 0);
	if (!po) {
		err = OSKIT_E_OUTOFMEMORY;
		goto finish;
	}
	po->ioi.ops = &block_io_ops;
	po->fops = blkdevs[MAJOR(kdev)].fops;
	po->count = 1;
	po->busy = 1;
	po->want = 0;
	po->mode = mode;
	init_waitqueue(&po->waitq);

	po->inode.i_rdev = kdev;

	po->inode.i_emul_data = po;

	po->next = open_list;
	open_list = po->next;

	if (!po->fops->open)
		err = 0;
	else {
		struct dentry dentry;
		file.f_mode = mode;
		file.f_flags = 0;
		file.f_dentry = &dentry;
		dentry.d_inode = &po->inode;

		err = (*po->fops->open)(&po->inode, &file);
	}

	if (err) {
		open_list_remove(po);
		if (po->want) {
			po->want = 0;
			wake_up(&po->waitq);
		}
		err = linux_to_oskit_error(err);
		oskit_linux_mem_free(po, OSENV_PHYS_WIRED,
				    sizeof(struct peropen));
	} else {
		/*
		 * Now that the device has been opened,
		 * we can extract its size.  This might
		 * not have been set properly in blk_size
		 * before the open call, e.g. for the floppy driver.
		 */
		if (gd)
			po->size = gd->part[MINOR(kdev)].nr_sects;
		else {
			osenv_assert(blk_size[MAJOR(kdev)] &&
				     blk_size[MAJOR(kdev)][MINOR(kdev)]);
			po->size = (blk_size[MAJOR(kdev)][MINOR(kdev)]
				    << (BLOCK_SIZE_BITS - DISK_BLOCK_BITS));
		}

		if (blksize_size[MAJOR(kdev)]
		    && blksize_size[MAJOR(kdev)][MINOR(kdev)]) {
			int bshift = DISK_BLOCK_BITS;
			po->bsize = blksize_size[MAJOR(kdev)][MINOR(kdev)];
			for (i = po->bsize >> DISK_BLOCK_BITS; i != 1; i >>= 1)
				bshift++;
			po->bshift = bshift;
		}
		else {
			/* XXX: Linux uses BLOCK_SIZE=1024, not 512 */
			po->bsize = DISK_BLOCK_SIZE;
			po->bshift = DISK_BLOCK_BITS;
		}
		po->bmask = po->bsize - 1;

		po->busy = 0;
		*out_io = &po->ioi;
		if (po->want) {
			po->want = 0;
			wake_up(&po->waitq);
		}
	}
 finish:
	OSKIT_LINUX_DESTROY_CURRENT();
	return (err);
}


/*** Methods implementing the per-open block I/O interface ***/

/*
 * Query a block I/O object for its interfaces.
 * This is extremely easy because we only export one interface
 * (plus its base type, IUnknown).
 */
static OSKIT_COMDECL
blkio_query(oskit_blkio_t *io, const struct oskit_guid *iid, void **out_ihandle)
{
	struct peropen *po;

	po = (struct peropen*)io;
	if (po == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);
	if (po->count == 0)
		panic("%s:%d: bad count", __FILE__, __LINE__);

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_blkio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &po->ioi;
		++po->count;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

/*
 * Clone a reference to a device's block I/O interface.
 */
static OSKIT_COMDECL_U
blkio_addref(oskit_blkio_t *io)
{
	struct peropen *po;

	po = (struct peropen*)io;
	if (po == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);
	if (po->count == 0)
		panic("%s:%d: bad count", __FILE__, __LINE__);

	return ++po->count;
}

/*
 * Close ("release") a device.
 */
static OSKIT_COMDECL_U
blkio_release(oskit_blkio_t *io)
{
	struct file file;
	struct peropen *po;
	unsigned newcount;
	struct task_struct ts;

	OSKIT_LINUX_CREATE_CURRENT(ts);

	po = (struct peropen*)io;
	if (po == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);
	if (po->count == 0)
		panic("%s:%d: bad count", __FILE__, __LINE__);

	while (po->busy) {
		po->want = 1;
		wake_up(&po->waitq);
	}
	newcount = --po->count;
	if (newcount == 0) {
		if (po->fops->release) {
			struct dentry dentry;
			file.f_mode = po->mode;
			file.f_flags = 0;
			file.f_dentry = &dentry;
			dentry.d_inode = &po->inode;
			po->busy = 1;
			(*po->fops->release)(&po->inode, &file);
		}
		open_list_remove(po);
		if (po->want) {
			po->want = 0;
			wake_up(&po->waitq);
		}
		oskit_linux_mem_free(po, OSENV_PHYS_WIRED,
				    sizeof(struct peropen));
	}

	OSKIT_LINUX_DESTROY_CURRENT();

	return newcount;
}

/*
 * Return the block size of this device
 */
static OSKIT_COMDECL_U
blkio_getblocksize(oskit_blkio_t *io)
{
	struct peropen *po;

	po = (struct peropen*)io;
	if (po == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);
	if (po->count == 0)
		panic("%s:%d: bad count", __FILE__, __LINE__);

	return po->bsize;
}

/*
 * Read from a device.
 */
static OSKIT_COMDECL
blkio_read(oskit_blkio_t *io, void *buf,
	   oskit_off_t off, oskit_size_t count, oskit_size_t *amount_read)
{
	int err;
	struct file file;
	struct dentry dentry;
	struct peropen *po;
	unsigned bn, sz;
	struct task_struct ts;

	po = (struct peropen*)io;
	if (po == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);
	if (!po->fops->read || po->mode == O_WRONLY)
		return (OSKIT_E_DEV_BADOP);
	bn = off >> DISK_BLOCK_BITS;
	if (count == 0 || bn == po->size) {
		*amount_read = 0;
		return (0);
	}
	if (bn > po->size)
		return (OSKIT_E_DEV_BADPARAM);

	OSKIT_LINUX_CREATE_CURRENT(ts);

	if ((off & (DISK_BLOCK_SIZE - 1)) == 0)
		sz = (count + (DISK_BLOCK_SIZE - 1)) >> DISK_BLOCK_BITS;
	else {
		sz = 1;
		if (count > DISK_BLOCK_SIZE - (off & (DISK_BLOCK_SIZE - 1)))
			sz += (count - (off & (DISK_BLOCK_SIZE - 1))
			       + (DISK_BLOCK_SIZE - 1)) >> DISK_BLOCK_BITS;
	}
	if (po->size - bn < sz)
		sz = po->size - bn;
	if ((sz << DISK_BLOCK_BITS) - (off & (DISK_BLOCK_SIZE - 1)) < count)
		count = (sz << DISK_BLOCK_BITS) - (off & (DISK_BLOCK_SIZE - 1));
	file.f_mode = po->mode;
	file.f_flags = 0;
	file.f_pos = off;
	file.f_inode = &po->inode;
	file.f_dentry = &dentry;
	dentry.d_inode = &po->inode;

	err = (*po->fops->read)(&file, buf, count, 0);
	if (err) {
		err = linux_to_oskit_error(err);
		*amount_read = 0;	/* XXX */
	} else
		*amount_read = count;

	OSKIT_LINUX_DESTROY_CURRENT();

	return (err);
}

/*
 * Write to a device.
 */
static OSKIT_COMDECL
blkio_write(oskit_blkio_t *io, const void *buf,
	oskit_off_t off, oskit_size_t count, oskit_size_t *amount_written)
{
	int err;
	struct file file;
	struct dentry dentry;
	struct peropen *po;
	unsigned bn, sz;
	struct task_struct ts;

	po = (struct peropen*)io;
	if (po == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);
	if (!po->fops->write || po->mode == O_RDONLY)
		return (OSKIT_E_DEV_BADOP);
	bn = off >> DISK_BLOCK_BITS;
	if (count == 0 || bn == po->size) {
		*amount_written = 0;
		return (0);
	}

	OSKIT_LINUX_CREATE_CURRENT(ts);

	if (bn > po->size)
		return (OSKIT_E_DEV_BADPARAM);
	if ((off & (DISK_BLOCK_SIZE - 1)) == 0)
		sz = (count + (DISK_BLOCK_SIZE - 1)) >> DISK_BLOCK_BITS;
	else {
		sz = 1;
		if (count > DISK_BLOCK_SIZE - (off & (DISK_BLOCK_SIZE - 1)))
			sz += (count - (off & (DISK_BLOCK_SIZE - 1))
			       + (DISK_BLOCK_SIZE - 1)) >> DISK_BLOCK_BITS;
	}
	if (po->size - bn < sz)
		sz = po->size - bn;
	if ((sz << DISK_BLOCK_BITS) - (off & (DISK_BLOCK_SIZE - 1)) < count)
		count = (sz << DISK_BLOCK_BITS) - (off & (DISK_BLOCK_SIZE - 1));
	file.f_mode = po->mode;
	file.f_flags = 0;
	file.f_pos = off;
	file.f_inode = &po->inode;
	file.f_dentry = &dentry;
	dentry.d_inode = &po->inode;
	err = (po->fops->write)(&file, buf, count, 0);
	if (err) {
		osenv_log(OSENV_LOG_NOTICE, "WRITE error %d, 0x%x\n", err, err);
		err = linux_to_oskit_error(err);
		*amount_written = 0;	/* XXX */
	} else
		*amount_written = count;

	OSKIT_LINUX_DESTROY_CURRENT();

	return (err);
}

static OSKIT_COMDECL
blkio_getsize(oskit_blkio_t *io, oskit_off_t *out_size)
{
	struct peropen *po;

	po = (struct peropen*)io;
	if (po == NULL)
		panic("%s:%d: null peropen", __FILE__, __LINE__);

	/* XXX The Linux ide driver sets the size to 0x7fffffff for
	 * IDE CD-ROM's and tape.  Ack!
	 *
	 * ref: linux/src/drivers/block/ide.c:current_capacity()
	 */
	if (po->size == 0x7fffffff)
		return OSKIT_E_NOTIMPL;

	/*
	 * peropen size is in 512 byte sectors.
	 * Note that without this cast, we truncate the result to 32 bits!
	 */
	*out_size = (oskit_off_t)po->size * 512;

	return 0;
}

static OSKIT_COMDECL
blkio_setsize(oskit_blkio_t *io, oskit_off_t new_size)
{
	return OSKIT_E_NOTIMPL;
}


/*** Methods implementing the default fdev block device interface ***/
/*
 * This implementation is only used for Linux block device drivers
 * that don't multiplex multiple actual devices based on minor number.
 */

static OSKIT_COMDECL
bdev_query(oskit_blkdev_t *fdev, const struct oskit_guid *iid, void **out_ihandle)
{
	int major = (struct device_struct*)fdev - blkdevs;

	if (major < 0 || major >= MAX_BLKDEV)
		panic("%s:%d: bad blkdev poitner", __FILE__, __LINE__);

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_device_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_blkdev_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &blkdevs[major].blki;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U bdev_addref(oskit_blkdev_t *dev)
{
	/* No reference counting for block device nodes */
	return 1;
}

static OSKIT_COMDECL_U bdev_release(oskit_blkdev_t *dev)
{
	/* No reference counting for block device nodes */
	return 1;
}

static OSKIT_COMDECL bdev_getinfo(oskit_blkdev_t *fdev, oskit_devinfo_t *out_info)
{
	int major = (struct device_struct*)fdev - blkdevs;

	if (major < 0 || major >= MAX_BLKDEV)
		panic("%s:%d: bad blkdev poitner", __FILE__, __LINE__);
	if (blkdevs[major].info == NULL)
		panic("%s:%d: no device info registered", __FILE__, __LINE__);

	*out_info = *blkdevs[major].info;
	return 0;
}

static OSKIT_COMDECL bdev_getdriver(oskit_blkdev_t *fdev,
				      oskit_driver_t **out_driver)
{
	int major = (struct device_struct*)fdev - blkdevs;

	if (major < 0 || major >= MAX_BLKDEV)
		panic("%s:%d: bad blkdev poitner", __FILE__, __LINE__);
	if (blkdevs[major].drv == NULL)
		panic("%s:%d: no driver registered", __FILE__, __LINE__);

	*out_driver = blkdevs[major].drv;
	oskit_driver_addref(blkdevs[major].drv);
	return 0;
}

static OSKIT_COMDECL bdev_open(oskit_blkdev_t *fdev, unsigned mode,
				oskit_blkio_t **out_blkio)
{
	int major = (struct device_struct*)fdev - blkdevs;

	if (major < 0 || major >= MAX_BLKDEV)
		panic("%s:%d: bad blkdev poitner", __FILE__, __LINE__);
	if (blkdevs[major].drv == NULL)
		panic("%s:%d: no driver registered", __FILE__, __LINE__);

	return oskit_linux_block_open_kdev(MKDEV(major, 0), NULL, mode,
					  out_blkio);
}
