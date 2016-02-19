/*
 * Copyright (c) 1997-1998, 2000 University of Utah and the Flux Group.
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
#include <oskit/types.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/dev.h>
#include <oskit/io/blkio.h>

#include "native.h"

oskit_error_t
oskit_linux_init_ide(void)
{
	return 0;
}

void
oskit_linux_init_scsi(void)
{
}

oskit_error_t
oskit_linux_init_floppy(void)
{
	return 0;
}

struct blkio_impl {
	oskit_blkio_t ioi;
	unsigned count;
	int fd;
	oskit_size_t blocksize;
	oskit_size_t size;		/* size in multiples of blocksize */
};
typedef struct blkio_impl blkio_impl_t;

#define SANITY(b) do {							\
	if ((b) == NULL)						\
		panic("%s:%d: null peropen", __FILE__, __LINE__);	\
	if ((b)->count == 0)						\
		panic("%s:%d: bad count", __FILE__, __LINE__);		\
} while(0)

/*
 * Query a block I/O object for its interfaces.
 * This is extremely easy because we only export one interface
 * (plus its base type, IUnknown).
 */
static OSKIT_COMDECL
blkio_query(oskit_blkio_t *io, const struct oskit_guid *iid, void **out_ihandle)
{
	blkio_impl_t *b = (blkio_impl_t *)io;

	SANITY(b);

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_blkio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &b->ioi;
		++b->count;
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
	blkio_impl_t *b = (blkio_impl_t *)io;

	SANITY(b);

	return ++b->count;
}

/*
 * Close ("release") a device.
 */
static OSKIT_COMDECL_U
blkio_release(oskit_blkio_t *io)
{
	blkio_impl_t *b = (blkio_impl_t *)io;
	unsigned newcount;

	SANITY(b);

	newcount = --b->count;
	if (b->count == 0) {
		NATIVEOS(close)(b->fd);
		free(b);
	}

	return newcount;
}

/*
 * Return the block size of this device
 */
static OSKIT_COMDECL_U
blkio_getblocksize(oskit_blkio_t *io)
{
	blkio_impl_t *b = (blkio_impl_t *)io;

	SANITY(b);

	return b->blocksize;
}

/*
 * Read from a device.
 */
static OSKIT_COMDECL
blkio_read(oskit_blkio_t *io, void *buf,
	   oskit_off_t off, oskit_size_t count, oskit_size_t *amount_read)
{
	blkio_impl_t *b = (blkio_impl_t *)io;
	int err;

	SANITY(b);

	err = NATIVEOS(lseek)(b->fd, off, SEEK_SET);
	if (err < 0) {
		oskitunix_perror("lseek");
		return OSKIT_EIO;
	}
	*amount_read = NATIVEOS(read)(b->fd, buf, count);
	if (*amount_read < 0) {
		oskitunix_perror("read");
		return OSKIT_EIO;
	}

	return 0;
}

/*
 * Write to a device.
 * Note that amount_written is unsigned.
 */
static OSKIT_COMDECL
blkio_write(oskit_blkio_t *io, const void *buf,
	    oskit_off_t off, oskit_size_t count, oskit_size_t *amount_written)
{
	blkio_impl_t *b = (blkio_impl_t *)io;
	ssize_t rval;
	int err;

	SANITY(b);

	err = NATIVEOS(lseek)(b->fd, off, SEEK_SET);
	if (err < 0) {
		oskitunix_perror("lseek");
		return OSKIT_EIO;
	}
	rval = NATIVEOS(write)(b->fd, buf, count);
	if (rval < 0) {
		oskitunix_perror("write");
		return OSKIT_EIO;
	}
	*amount_written = rval;

	return 0;
}

/*
 * Get the size of this block io object in bytes.
 */
static OSKIT_COMDECL
blkio_getsize(oskit_blkio_t *io, oskit_off_t *out_size)
{
	blkio_impl_t *b = (blkio_impl_t *)io;

	SANITY(b);

	*out_size = b->size * b->blocksize;
	return 0;
}

static OSKIT_COMDECL
blkio_setsize(oskit_blkio_t *io, oskit_off_t new_size)
{
	return OSKIT_E_NOTIMPL;
}

static struct oskit_blkio_ops blkio_ops = {
	blkio_query,
	blkio_addref,
	blkio_release,
	blkio_getblocksize,
	blkio_read,
	blkio_write,
	blkio_getsize,
	blkio_setsize
};

oskit_error_t
oskit_linux_block_open(const char *name, unsigned flags, oskit_blkio_t **out_io)
{
	blkio_impl_t *b;
	int fd;
	int unix_flags;

	if (flags & ~OSKIT_DEV_OPEN_ALL)
		return OSKIT_E_DEV_BADPARAM;
	switch (flags & (OSKIT_DEV_OPEN_READ|OSKIT_DEV_OPEN_WRITE)) {
	case OSKIT_DEV_OPEN_READ:
		unix_flags = O_RDONLY;
		break;
	case OSKIT_DEV_OPEN_WRITE:
		unix_flags = O_WRONLY;
		break;
	case OSKIT_DEV_OPEN_READ|OSKIT_DEV_OPEN_WRITE:
		unix_flags = O_RDWR;
		break;
	default:
		return OSKIT_E_DEV_BADPARAM;
	}

	fd = NATIVEOS(open)(name, unix_flags);
	if (fd < 0) {
		oskitunix_perror("open");
		return OSKIT_E_DEV_IOERR;
	}

	b = malloc(sizeof(blkio_impl_t));
	if (b == NULL)
		return OSKIT_E_OUTOFMEMORY;
	b->ioi.ops = &blkio_ops;
	b->count = 1;
	b->fd = fd;
	b->blocksize = 1024;
	b->size = NATIVEOS(lseek)(fd, SEEK_END, 0);

	*out_io = &b->ioi;

	return 0;
}
