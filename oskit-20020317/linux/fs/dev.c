/*
 * Copyright (c) 1997, 1998, 1999 The University of Utah and the Flux Group.
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
 * Device related stuff.
 */

#include <oskit/io/blkio.h>
#include <assert.h>

#include <linux/fs.h>
#include <linux/blk.h>
#include <linux/malloc.h>
#include <linux/sched.h>

#include "dev.h"

#if 0
# define debugf(fmt, args...) printk(fmt , ## args)
#else
# define debugf(fmt, args...)
#endif


/*
 * The devtab is indexed by major number.
 * This is for fs_linux_devtab_{insert,delete},
 * it has no Linux significance.
 */
static oskit_blkio_t *devtab[MAX_BLKDEV] = { NULL, NULL, };

/*
 * This is used by blkdev_open and contains the size/blksize/hardsectsize
 * of each dev,
 * it has no Linux significance.
 */
static int sizes[MAX_BLKDEV][3];


/* These global vars are from Linux and used in the filesystems code. */

/*
 * This specifies how many sectors to read ahead on the disk.
 */
int read_ahead[MAX_BLKDEV] = {0, };

/*
 * blk_size contains the size of all block-devices in units of 1024 byte
 * sectors:
 *
 * blk_size[MAJOR][MINOR]
 *
 * if (!blk_size[MAJOR]) then no minor size checking is done.
 */
int *blk_size[MAX_BLKDEV] = { NULL, NULL, };

/*
 * blksize_size contains the size of all block-devices:
 *
 * blksize_size[MAJOR][MINOR]
 *
 * if (!blksize_size[MAJOR]) then 1024 bytes is assumed.
 */
int *blksize_size[MAX_BLKDEV] = { NULL, NULL, };

/*
 * hardsect_size contains the size of the hardware sector of a device.
 *
 * hardsect_size[MAJOR][MINOR]
 *
 * if (!hardsect_size[MAJOR])
 *		then 512 bytes is assumed.
 * else
 *		sector_size is hardsect_size[MAJOR][MINOR]
 * This is currently set by some scsi device and read by the msdos fs driver
 * This might be a some uses later.
 */
int *hardsect_size[MAX_BLKDEV] = { NULL, NULL, };


void
fs_linux_dev_init()
{

	/*
	 * ROOT_DEV is only used for comparison in special cases of
	 * Linux functions.
	 * We make it NODEV so those comparisons always fail.
	 */
	ROOT_DEV = NODEV;
}


/* devtab stuff */

/*
 * Major number values in this library have no correlation to Linux
 * major numbers except the range.
 * fs_linux_mount picks the first empty slot after 0 in `devtab' and takes that
 * index as the major number for the kdev_t that it creates.
 * The minor number is always zero.
 * We don't allow zero major numbers because the zero kdev_t has
 * special meaning in Linux.
 */

oskit_error_t
fs_linux_devtab_insert(oskit_blkio_t *bio, kdev_t *devp)
{
	int i;
	struct task_struct *ts;

	for (i = 1; i < MAX_BLKDEV; i++)
		if (devtab[i] == NULL) {
			devtab[i] = bio;
			ts = current;
			oskit_blkio_addref(bio);
			current = ts;
			*devp = MKDEV(i, 0);
			return 0;
		}
	return OSKIT_ENOMEM;
}

void
fs_linux_devtab_delete(kdev_t dev)
{
	int i;
	struct task_struct *ts;

	i = MAJOR(dev);
	assert(i > 0 && i <= MAX_BLKDEV);
	ts = current;
	oskit_blkio_release(devtab[i]);
	current = ts;
	devtab[i] = NULL;
}


/*
 * Opens the device specified by the kdev_t in inode->i_rdev.
 *
 * Linux: this calls the open op that should have been registered for this
 * device by register_blkdev by the driver.
 *
 * Oskit: the device was already opened by the caller of fs_linux_mount
 * so all we have to do is fill in the tables like blk_size, etc,
 * which are normally initialized by drivers.
 * Note that since we call oskit_blkio_getsize we have to save `current' around
 * it since oskit_blkio_write from the Linux fdev may use current.
 *
 * This returns 0 on success.
 */
int
blkdev_open(struct inode * inode, struct file * filp)
{
	kdev_t dev = inode->i_rdev;
	oskit_blkio_t *bio = devtab[MAJOR(dev)];
	oskit_off_t part_bytes;
	oskit_error_t err;
	struct task_struct *ts;

	/*
	 * Read ahead amount.
	 *
	 * We have no way of knowing this, so we leave it zero.
	 */
	read_ahead[MAJOR(dev)] = 0;

	/*
	 * Size of device in 1k sectors.
	 */
	ts = current;
	err = oskit_blkio_getsize(bio, &part_bytes);
	current = ts;
	if (err) {
		printk("%s: oskit_blkio_getsize failed 0x%x\n",
		       __FUNCTION__, err);
		return -EIO;
	}
	sizes[MAJOR(dev)][0] = (part_bytes << BLOCK_SIZE_BITS);
	blk_size[MAJOR(dev)] = &sizes[MAJOR(dev)][0];

	/*
	 * Block size used for this device,
	 * doesn't have to equal hardsect_size.
	 *
	 * We just need to allocate storage for this and set it to something
	 * reasonable.
	 * The filesystem read_super routines set this to the filesystem's
	 * actual blocksize.
	 */
	sizes[MAJOR(dev)][1] = BLOCK_SIZE;
	blksize_size[MAJOR(dev)] = &sizes[MAJOR(dev)][1];

	/*
	 * Hardware sector size.
	 */
	ts = current;
	sizes[MAJOR(dev)][2] = oskit_blkio_getblocksize(bio);
	current = ts;
	hardsect_size[MAJOR(dev)] = &sizes[MAJOR(dev)][2];

	return 0;
}


/*
 * This function can be used to request a number of buffers
 * from a block device.
 * Currently the only restriction is that all buffers
 * must belong to the same device.
 * It is called when the buffer cache doesn't have our block.
 *
 * Linux: calls make_request for each buffer wanted and doesn't
 * attempt any buffer coalescing,
 * which is done in make_request on the list of requests.
 *
 * Oskit: We just oskit_blkio_read/write for each one.
 * We don't attempt any coalescing but I guess we could try it on the
 * buffer_head list.
 * Note that since we call oskit_blkio ops we have to save `current' around
 * them since oskit_blkio_write from the Linux fdev uses current.
 */
void
ll_rw_block(int rw, int nr, struct buffer_head *bh[])
{
	int i, correct_size;
	oskit_error_t err;
	oskit_size_t actual;
	oskit_blkio_t *blkdev;
	kdev_t dev;
	const char *op;
	struct task_struct *ts;

	/* Make sure that the first block contains something reasonable */
	while (!*bh) {
		bh++;
		if (--nr <= 0)
			return;
	}

	/* Get our oskit_blkio_t. */
	dev = bh[0]->b_dev;
	blkdev = devtab[MAJOR(dev)];
	if (blkdev == NULL) {
		printk(KERN_ERR "ll_rw_block: no devtab entry\n");
		goto sorry;
	}

	/* Determine correct block size for this device.  */
	correct_size = BLOCK_SIZE;
	if (blksize_size[MAJOR(dev)]) {
		i = blksize_size[MAJOR(dev)][MINOR(dev)];
		if (i)
			correct_size = i;
	}

	for (i = 0; i < nr; i++) {
		if (bh[i] == NULL)
			continue;

		/* Make sure buffer size agrees with device. */
		if (bh[i] && bh[i]->b_size != correct_size) {
			printk(KERN_NOTICE "ll_rw_block: device %s: "
			       "only %d-char blocks implemented (%lu)\n",
			       kdevname(dev),
			       correct_size, bh[i]->b_size);
			goto sorry;
		}

		/* I don't get this but Linux does it before make_request. */
		set_bit(BH_Req, &bh[i]->b_state);

		/*
		 * This part is modeled after Linux's
		 *	make_request -> add_request -> request_fn
		 * code.
		 */
		lock_buffer(bh[i]);
		err = 0;
		actual = correct_size;
		switch (rw) {
		case READ:
		case READA:
			op = "read";
			if (buffer_uptodate(bh[i]))
				break;
			debugf(__FUNCTION__": reading block %d from dev %#x\n",
			       bh[i]->b_blocknr, dev);
			ts = current;
			err = oskit_blkio_read(blkdev, bh[i]->b_data,
					      bh[i]->b_blocknr * correct_size,
					      correct_size, &actual);
			current = ts;
			break;
		case WRITE:
		case WRITEA:
			op = "write";
			if (! buffer_dirty(bh[i]))
				break;
			debugf(__FUNCTION__": writing block %d from dev %#x\n",
			       bh[i]->b_blocknr, dev);
			ts = current;
			err = oskit_blkio_write(blkdev, bh[i]->b_data,
					      bh[i]->b_blocknr * correct_size,
					      correct_size, &actual);
			current = ts;
			break;
		default:
			printk(KERN_ERR "%s: invalid rw flag 0x%x\n",
			       __FUNCTION__, rw);
			unlock_buffer(bh[i]);
			return;		/* no use trying again */
		}
		if (err) {
			printk(KERN_ERR "%s: oskit_blkio_%s error 0x%x\n",
			       __FUNCTION__, op, err);
			unlock_buffer(bh[i]);
			continue;
		}
		if (actual != correct_size)
			printk(KERN_WARNING
			       "%s: oskit_blkio_%s: %d expected %d\n",
			       __FUNCTION__, op, actual, correct_size);
		mark_buffer_clean(bh[i]);
		mark_buffer_uptodate(bh[i], 1);
		unlock_buffer(bh[i]);
	}

	return;

 sorry:
	for (i = 0; i < nr; i++) {
		if (bh[i]) {
			clear_bit(BH_Dirty, &bh[i]->b_state);
			clear_bit(BH_Uptodate, &bh[i]->b_state);
		}
	}
}


/*
 * generic_file_read()
 *
 * Linux: looks in the VM page cache else calls i_op->read_page
 * for each page of the file.
 * i_op->read_page is usually generic_read_page,
 * which does roughly what we do here: call bmap and then read the block.
 *
 * Oskit: we just call bread on each logical block needed.
 * If we wanted to improve this we could call bmap for all blocks and then do
 * sorting and coalesing before bread'ing.
 */

#define bround_down(pos, inode)	((pos) >> (inode)->i_sb->s_blocksize_bits)
#define bround_up(pos, inode)	(bround_down(pos, inode) + 1)
#undef min
#define min(a,b)		((a) < (b) ? (a) : (b))

ssize_t
generic_file_read(struct file *filp, char *buf, size_t count, loff_t *off)
{
	struct inode *inode = filp->f_dentry->d_inode;
	int block, start_block, nblocks;
	int physblock;
	int bytesleft;
	int offset;
	int blocksize;
	int amt;
	struct buffer_head *bh;
	int retval;

	if (count == 0)
		return 0;
	if (inode == NULL || filp == NULL || buf == NULL ||
	    inode->i_op == NULL || inode->i_op->bmap == NULL ||
	    inode->i_sb == NULL)
		return -EINVAL;

	filp->f_pos = *off;

	/*
	 * Figure out which logical blocks of the file to read.
	 * Make sure nblocks doesn't go past EOF.
	 */
	start_block = bround_down(filp->f_pos, inode);
	nblocks = (bround_up(min(filp->f_pos + count, inode->i_size) - 1, inode)
		   - start_block);
	count = min(count, inode->i_size - filp->f_pos);

	/*
	 * For each logical block,
	 * find the physical block via bmap,
	 * read it via bread,
	 * copy it into buf somewhere.
	 *
	 * `offset' is how far into the first block f_pos is.
	 */
	blocksize = inode->i_sb->s_blocksize;
	offset = filp->f_pos - blocksize*start_block;
	bytesleft = count;
	for (block = start_block; nblocks != 0; block++, nblocks--) {
		physblock = inode->i_op->bmap(inode, block);
		if (physblock == 0) {
			/* Hole. */
			amt = min(blocksize, bytesleft);
			memset(buf, 0, amt);
		}
		else {
			bh = bread(inode->i_dev, physblock, blocksize);
			if (bh == NULL) {
				retval = -EIO;
				goto done;
			}
			amt = min(bh->b_size - offset, bytesleft);
			memcpy(buf, bh->b_data + offset, amt);
			brelse(bh);
		}
		buf += amt;
		bytesleft -= amt;
		offset = 0;
	}

	retval = count - bytesleft;
	filp->f_pos += retval;	/* update seek pointer */

 done:
	if (bytesleft != count && ! IS_RDONLY(inode)) {
		/* Then we have read something -- update the atime. */
		inode->i_atime = CURRENT_TIME;
		mark_inode_dirty(inode);
	}
	return retval;
}
