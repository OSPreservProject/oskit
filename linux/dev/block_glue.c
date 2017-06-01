/*
 * Copyright (c) 1996-2001 The University of Utah and the Flux Group.
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
 * Glue for Linux block drivers.
 */

#include <oskit/c/assert.h>

#include <linux/fs.h>
#include <linux/blk.h>
#include <linux/fcntl.h>
#include <linux/major.h>
#include <linux/kdev_t.h>
#include <linux/string.h>

#include "block.h"
#include "irq.h"
#include "shared.h"
#include "osenv.h"

/*
 * Maximum number of block buffers we allocate at a time.
 */
#define MAX_BUF	128

/*
 * Flags for osenv_mem_alloc when allocating a buffer.
 */
#define BUF_MEM_FLAGS	(OSENV_PHYS_WIRED|OSENV_PHYS_CONTIG|OSENV_VIRT_EQ_PHYS)

/*
 * Driver file operations table.
 */
struct device_struct blkdevs[MAX_BLKDEV];

#if 1
/* blk_dev_struct is:
 *      *request_fn
 *      *current_request
 */
struct blk_dev_struct blk_dev[MAX_BLKDEV]; /* initialized by blk_dev_init() */
#else
/*
 * Driver request function table.
 */
struct blk_dev_struct blk_dev[MAX_BLKDEV] = {
	{ NULL, NULL },		/* 0 no_dev */
	{ NULL, NULL },		/* 1 dev mem */
	{ NULL, NULL },		/* 2 dev fd */
	{ NULL, NULL },		/* 3 dev ide0 or hd */
	{ NULL, NULL },		/* 4 dev ttyx */
	{ NULL, NULL },		/* 5 dev tty */
	{ NULL, NULL },		/* 6 dev lp */
	{ NULL, NULL },		/* 7 dev pipes */
	{ NULL, NULL },		/* 8 dev sd */
	{ NULL, NULL },		/* 9 dev st */
	{ NULL, NULL },		/* 10 */
	{ NULL, NULL },		/* 11 */
	{ NULL, NULL },		/* 12 */
	{ NULL, NULL },		/* 13 */
	{ NULL, NULL },		/* 14 */
	{ NULL, NULL },		/* 15 */
	{ NULL, NULL },		/* 16 */
	{ NULL, NULL },		/* 17 */
	{ NULL, NULL },		/* 18 */
	{ NULL, NULL },		/* 19 */
	{ NULL, NULL },		/* 20 */
	{ NULL, NULL },		/* 21 */
	{ NULL, NULL }		/* 22 dev ide1 */
};
#endif

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

/*
 * This specifies how many sectors to read ahead on the disk.
 * This is unused in the OSKIT.  It is here to make drivers compile.
 */
int read_ahead[MAX_BLKDEV] = {0, };

/*
 * The following tunes the read-ahead algorithm in mm/filemap.c
 * This is unused in the OSKIT.  It is here to make drivers compile.
 */
int * max_readahead[MAX_BLKDEV] = { NULL, NULL, };

/*
 * Max number of sectors per request
 * This is unused in the OSKIT.  It is here to make drivers compile.
 */
int * max_sectors[MAX_BLKDEV] = { NULL, NULL, };

/*
 * Wait queue on which drivers sleep waiting for request structures.
  */
struct wait_queue *wait_for_request = NULL;

/*
 * Is called with the request spinlock aquired.
 * NOTE: the device-specific queue() functions
 * have to be atomic!
 */
static inline struct request **
get_queue(kdev_t dev)
{
	struct blk_dev_struct *bdev = blk_dev + MAJOR(dev);

	if (bdev->queue)
		return bdev->queue(dev);
	return &(bdev->current_request);
}

/*
 * Enqueue an I/O request on a driver's queue.
 */
static inline void
enqueue_request(struct request *req)
{
	struct blk_dev_struct *dev;
	struct request *tmp, **current_request;

	dev = blk_dev + MAJOR(req->rq_dev);

	linux_cli();

	current_request = get_queue(req->rq_dev);
	tmp = *current_request;
	if (!tmp) {
		*current_request = req;
		(*dev->request_fn)();

		linux_sti();

		return;
	}
	while (tmp->next) {
		if ((IN_ORDER(tmp, req) || !IN_ORDER(tmp, tmp->next))
		    && IN_ORDER(req, tmp->next))
			break;
		tmp = tmp->next;
	}
	req->next = tmp->next;
	tmp->next = req;
	if (scsi_blk_major(MAJOR(req->rq_dev)))
		(*dev->request_fn)();

	linux_sti();
}

#define FREE_BUFFERS(num) {						\
	oskit_linux_mem_free(b_data, BUF_MEM_FLAGS, po->bsize * num);	\
	oskit_linux_mem_free(bhp, OSENV_PHYS_WIRED,			\
		      (sizeof(struct buffer_head *)			\
		       + sizeof(struct buffer_head)) * nbuf);		\
	if (req)							\
		oskit_linux_mem_free(req, OSENV_PHYS_WIRED, sizeof(*req));\
}

/*
 * Read/write data from/to a device.
 */
static int
block_io(int rw, struct inode *inode, struct file *filp, char *buf, int count)
{
	int err, i, nb, nbuf, resid;
	struct buffer_head *bh, **bhp;
	struct peropen *po;
	struct request *req;
	struct semaphore sem = MUTEX_LOCKED;
	unsigned blk, nsect, off;
	char *b_data;

	if (count < 0)
		return (-EINVAL);

	po = inode->i_emul_data;
	resid = count;
	blk = filp->f_pos >> po->bshift;
	off = 0;
	err = 0;
	req = NULL;

	assert((count & po->bmask) == 0);
	assert((filp->f_pos & po->bmask) == 0);

	/*
	 * Read/write full sectors.
	 * XXX: Get rid of `512'!!!
	 */
	if (resid >= 512) {
		/*
		 * Allocate buffers.
		 */
		nsect = resid >> 9;
		nbuf = (nsect + (po->bsize >> 9) - 1) / (po->bsize >> 9);
		if (nbuf > MAX_BUF)
			nbuf = MAX_BUF;
		bhp = oskit_linux_mem_alloc((sizeof(struct buffer_head *)
				      + sizeof(struct buffer_head)) * nbuf,
				     OSENV_PHYS_WIRED, 0);
		if (!bhp)
			return (-ENOMEM);
		bh = (struct buffer_head *)(bhp + nbuf);

		b_data = oskit_linux_mem_alloc(po->bsize * nbuf,
					       BUF_MEM_FLAGS, po->bsize);
		for (i = 0; i < nbuf; i++) {
			bhp[i] = bh + i;
			bhp[i]->b_dev = inode->i_rdev;
			bhp[i]->b_wait = NULL;
			bhp[i]->b_data = b_data + (po->bsize * i);
			if (!bh->b_data) {
				FREE_BUFFERS(i);
				return (-ENOMEM);
			}
		}
		req = oskit_linux_mem_alloc(sizeof(*req), OSENV_PHYS_WIRED, 0);
		if (!req) {
			FREE_BUFFERS(nbuf);
			return (-ENOMEM);
		}
		do {
			/*
			 * Construct buffer list.
			 */
			req->nr_sectors = 0;
			for (i = 0; i < nbuf && nsect; i++) {
				bhp[i]->b_state = 1 << BH_Lock;
				bhp[i]->b_blocknr = blk + i;
				bhp[i]->b_reqnext = bhp[i] + 1;
				if (po->bsize <= (nsect << 9))
					bhp[i]->b_size = po->bsize;
				else
					bhp[i]->b_size = nsect << 9;
				if (rw == WRITE) {
					bhp[i]->b_state |= 1 << BH_Dirty;
					memcpy(bhp[i]->b_data,
					       buf + off,
					       bhp[i]->b_size);
					off += bhp[i]->b_size;
				}
				req->nr_sectors += bhp[i]->b_size >> 9;
				nsect -= bhp[i]->b_size >> 9;
				resid -= bhp[i]->b_size;
			}


			bhp[i-1]->b_reqnext = NULL;
			nb = i;

			/*
			 * Construct request.
			 */
			req->sector = blk << (po->bshift - 9);
			req->current_nr_sectors = bhp[0]->b_size >> 9;
			req->buffer = bhp[0]->b_data;
			req->rq_status = RQ_ACTIVE;
			req->rq_dev = inode->i_rdev;
			req->cmd = rw;
			req->errors = 0;
			req->sem = &sem;
			req->bh = bhp[0];
			req->bhtail = bhp[nb - 1];
			req->next = NULL;

			blk += nb;

			/*
			 * Enqueue request and wait for I/O to complete.
			 */
			enqueue_request(req);
			/* XXX???? WHY was calling __down ? */
			down(&sem);
			if (req->errors) {
				FREE_BUFFERS(nbuf);
				return (-EIO);
			}

			if (rw == READ) {
				for (i = 0; i < nb; i++) {
				        memcpy(buf+off, bhp[i]->b_data,
					       bhp[i]->b_size);
					off += bhp[i]->b_size;
				}
			}
		} while (nsect);

		FREE_BUFFERS(nbuf);
	}

	return 0;
}

/*
 * Generic routine to read from a block device.
 */
int
block_read(struct file *file, char *buf, size_t count, loff_t *off)
{
	return (block_io(READ, file->f_inode, file, buf, count));
}

/*
 * Generic routine to write to a block device.
 */
int
block_write(struct file *file, const char *buf, size_t count, loff_t *off)
{
	return (block_io(WRITE, file->f_inode, file, (char*)buf, count));
}

/*
 * Register a block driver.
 */
int
register_blkdev(unsigned major, const char *name, struct file_operations *fops)
{
	if (major == 0) {
		for (major = MAX_BLKDEV - 1; major > 0; major--)
			if (!blkdevs[major].fops)
				break;
		if (major == 0)
			return (-EBUSY);
	} else {
		if (major >= MAX_BLKDEV)
			return (-EINVAL);
		if (blkdevs[major].fops && blkdevs[major].fops != fops)
			return (-EBUSY);
	}

	blkdevs[major].name = name;
	blkdevs[major].fops = fops;
	return (0);
}

/*
 * Unregister a block driver.
 */
int
unregister_blkdev(unsigned major, const char *name)
{
	if (major >= MAX_BLKDEV
	    || !blkdevs[major].fops
	    || strcmp(blkdevs[major].name, name))
		return (-EINVAL);

	blkdevs[major].fops = NULL;
	return (0);
}

/*
 * Allocate a block buffer.
 */
struct buffer_head *
getblk(kdev_t dev, int block, int size)
{
	struct buffer_head *bh;

	bh = oskit_linux_mem_alloc(sizeof(*bh), OSENV_PHYS_WIRED, 0);
	if (!bh)
		return (NULL);
	bh->b_data = oskit_linux_mem_alloc(size, BUF_MEM_FLAGS, size);
	if (!bh->b_data) {
		oskit_linux_mem_free(bh, OSENV_PHYS_WIRED, sizeof(*bh));
		return (NULL);
	}
	bh->b_dev = dev;
	bh->b_size = size;
	bh->b_state = 1 << BH_Lock;
	bh->b_blocknr = block;
	bh->b_wait = NULL;
	bh->b_reqnext = NULL;
	return (bh);
}

/*
 * Release a buffer.
 */
void
__brelse(struct buffer_head *bh)
{
	oskit_linux_mem_free(bh->b_data, BUF_MEM_FLAGS, bh->b_size);
	oskit_linux_mem_free(bh, OSENV_PHYS_WIRED, sizeof(*bh));
}

/*
 * Read data from a device.
 */
struct buffer_head *
bread(kdev_t dev, int block, int size)
{
	struct buffer_head *bh;

	bh = getblk(dev, block, size);
	if (!bh)
		return (NULL);
	ll_rw_block(READ, 1, &bh);
	wait_on_buffer(bh);
	if (!buffer_uptodate(bh)) {
		__brelse(bh);
		return (NULL);
	}
	return (bh);
}

#define NREQ	50

/*
 * Perform I/O request on a list of buffers.
 */
void
ll_rw_block(int rw, int nr, struct buffer_head **bh)
{
	int i, bsize, bshift;
	struct request *req;
	static int initdone = 0;
	static struct request requests[NREQ];	/* XXX */
#if 0
	struct wait_queue wait = { current, NULL };
#endif

	if (!initdone) {
		for (i = 0; i < NREQ; i++)
			requests[i].rq_status = RQ_INACTIVE;
		initdone = 1;
	}

	/*
	 * Find an unused request structure.
	 */
	linux_cli();

	while (1) {
		for (i = 0; i < NREQ; i++)
			if (requests[i].rq_status == RQ_INACTIVE)
				break;
		if (i < NREQ)
			break;
		sleep_on(&wait_for_request);
	}

	linux_sti();

	req = &requests[i];

	/*
	 * Compute device block size.
	 */
	bsize = BLOCK_SIZE;
	if (blksize_size[MAJOR(bh[0]->b_dev)]
	    && blksize_size[MAJOR(bh[0]->b_dev)][MINOR(bh[0]->b_dev)])
		bsize = blksize_size[MAJOR(bh[0]->b_dev)][MINOR(bh[0]->b_dev)];
	for (i = bsize, bshift = 0; i != 1; bshift++, i >>= 1)
		;

	/*
	 * Construct request.
	 */
	for (i = 0, req->nr_sectors = 0; i < nr - 1; i++) {
		req->nr_sectors += bh[i]->b_size >> 9;
		bh[i]->b_reqnext = bh[i + 1];
	}
	req->nr_sectors += bh[i]->b_size >> 9;
	bh[i]->b_reqnext = NULL;
	req->rq_status = RQ_ACTIVE;
	req->rq_dev = bh[0]->b_dev;
	req->cmd = rw;
	req->errors = 0;
	req->sector = bh[0]->b_blocknr << (bshift - 9);
	req->current_nr_sectors = bh[0]->b_size >> 9;
	req->buffer = bh[0]->b_data;
	req->bh = bh[0];
	req->bhtail = bh[nr - 1];
	req->next = NULL;

	/*
	 * Queue request.
	 */
	enqueue_request(req);
}

/*
 * This routine checks whether a removable media has been changed,
 * and invalidates all buffer-cache-entries in that case. This
 * is a relatively slow routine, so we have to try to minimize using
 * it. Thus it is called only upon a 'mount' or 'open'. This
 * is the best way of combining speed and utility, I think.
 * People changing diskettes in the middle of an operation deserve
 * to loose :-)
 */
int
check_disk_change(kdev_t dev)
{
	unsigned i;
	struct file_operations *fops;

	i = MAJOR(dev);
	if (i >= MAX_BLKDEV || (fops = blkdevs[i].fops) == NULL)
		return (0);
	if (fops->check_media_change == NULL)
		return (0);
	if (!(*fops->check_media_change)(dev))
		return (0);
	osenv_log(OSENV_LOG_NOTICE, "Disk change detected on device %s\n",
		kdevname(dev));
	if (fops->revalidate)
		(*fops->revalidate)(dev);

	return (1);
}

#undef  NR_REQUEST
#define NR_REQUEST  64
static struct request all_requests[NR_REQUEST];

/* RO fail safe mechanism */
static long ro_bits[MAX_BLKDEV][8];

int
is_read_only(kdev_t dev)
{
	int minor,major;

	major = MAJOR(dev);
	minor = MINOR(dev);
	if (major < 0 || major >= MAX_BLKDEV)
		return (0);
	return (ro_bits[major][minor >> 5] & (1 << (minor & 31)));
}

void
set_device_ro(kdev_t dev,int flag)
{
	int minor,major;

	major = MAJOR(dev);
	minor = MINOR(dev);
	if (major < 0 || major >= MAX_BLKDEV)
		return;
	if (flag)
		ro_bits[major][minor >> 5] |= 1 << (minor & 31);
	else
		ro_bits[major][minor >> 5] &= ~(1 << (minor & 31));
}

/*
 * Called by genhd.c to initialize block devices.
 */
int
blk_dev_init(void)
{
	struct request * req;
	struct blk_dev_struct *dev;

	for (dev = blk_dev + MAX_BLKDEV; dev-- != blk_dev;) {
		dev->request_fn      = NULL;
		dev->current_request = NULL;
		dev->queue           = NULL;
/* added these; changes in linux init */
		dev->plug.rq_status  = RQ_INACTIVE;
		dev->plug.cmd        = -1;
		dev->plug.next       = NULL;
		dev->plug_tq.sync    = 0;	/* XXX */
		dev->plug_tq.routine = unplug_device;	/* XXX */
		dev->plug_tq.data    = dev;
	}

	req = all_requests + NR_REQUEST;
	while (--req >= all_requests) {
		req->rq_status = RQ_INACTIVE;
		req->next = NULL;
	}
	memset(ro_bits,0,sizeof(ro_bits));

	return 0;
}

/*
 * This routine from linux/drivers/block/ll_rw_blk.c.
 */
int
end_that_request_first( struct request *req, int uptodate, char *name )
{
	struct buffer_head * bh;
	int nsect;

	req->errors = 0;
	if (!uptodate) {
		printk(KERN_ERR
		       "end_request: I/O error, dev %s (%s), sector %lu\n",
		       kdevname(req->rq_dev), name, req->sector);
#ifdef OSKIT
		nsect = 0;
		while (req->bh) {
			bh = req->bh;
			req->bh = bh->b_reqnext;
			mark_buffer_uptodate(bh, 0);
			unlock_buffer(bh);
		}
		return 0;
#else
		if ((bh = req->bh) != NULL) {
			nsect = bh->b_size >> 9;
			req->nr_sectors--;
			req->nr_sectors &= ~(nsect - 1);
			req->sector += nsect;
			req->sector &= ~(nsect - 1);
		}
#endif
	}

	if ((bh = req->bh) != NULL) {
		req->bh = bh->b_reqnext;
		bh->b_reqnext = NULL;
#ifdef OSKIT
		mark_buffer_uptodate(bh, uptodate);
		unlock_buffer(bh);
#else
		bh->b_end_io(bh, uptodate);
#endif
		if ((bh = req->bh) != NULL) {
			req->current_nr_sectors = bh->b_size >> 9;
			if (req->nr_sectors < req->current_nr_sectors) {
				req->nr_sectors = req->current_nr_sectors;
				printk("end_request: buffer-list destroyed\n");
			}
			req->buffer = bh->b_data;
			return 1;
		}
	}
	return 0;
}

/*
 * This routine from linux/drivers/block/ll_rw_blk.c.
 */
void
end_that_request_last( struct request *req )
{
	if (req->sem != NULL)
		up(req->sem);
	req->rq_status = RQ_INACTIVE;
	wake_up(&wait_for_request);
#ifdef OSKIT
	{
		unsigned long flags;
		save_flags(flags);
		cli();
		(*blk_dev[MAJOR(req->rq_dev)].request_fn)();
		restore_flags(flags);
	}
#endif
}

/*
 * Dummy do-nothing functions whose purpose
 * is merely to satisfy undefined references from Linux code
 * without having to change the Linux code itself.
 */
void chr_dev_init(void) { }
void net_dev_init(void) { }
void console_map_init(void) { }

void unplug_device(void * data)
{
	printk(KERN_CRIT "Unplug device called\n");
}
