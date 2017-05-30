/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
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
 * Implementation of simple oskit_blkio subsetting objects,
 * e.g., for representing individual partitions on a disk.
 */

#include <stdlib.h>
#include <string.h>

#include <oskit/io/blkio.h>

struct sub {
	oskit_blkio_t 	ioi;	/* Interface to this object */
	oskit_u32_t	refs;	/* Our reference count */
	oskit_blkio_t	*io;	/* Reference to underlying I/O object */
	oskit_off_t	offset;	/* Offset in underlying object */
	oskit_off_t	size;	/* Size of our I/O segment */
};

/*
 * Query a subset I/O object for its interfaces.
 * This is extremely easy because we only export one interface
 * (plus its base type, IUnknown).
 */
static OSKIT_COMDECL
query(oskit_blkio_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
	struct sub *s = (struct sub*)io;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_blkio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &s->ioi;
		++s->refs;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

/*
 * Clone a reference to our block I/O interface.
 */
static OSKIT_COMDECL_U
addref(oskit_blkio_t *io)
{
	struct sub *s = (struct sub*)io;

	return ++s->refs;
}

/*
 * Release a reference to our I/O interface.
 */
static OSKIT_COMDECL_U
release(oskit_blkio_t *io)
{
	struct sub *s = (struct sub*)io;

	if (--s->refs > 0)
		return s->refs;

	oskit_blkio_release(s->io);
	free(s);
	return 0;
}

/*
 * Return the block size of this block I/O object.
 */
static OSKIT_COMDECL_U
getblocksize(oskit_blkio_t *io)
{
	struct sub *s = (struct sub*)io;

	return oskit_blkio_getblocksize(s->io);
}

/*
 * Read from the block I/O subset object,
 * clipping accesses to the range of this subset.
 */
static OSKIT_COMDECL
read(oskit_blkio_t *io, void *buf,
     oskit_off_t off, oskit_size_t count, oskit_size_t *amount)
{
	struct sub *s = (struct sub*)io;

	if (off >= s->size) {
		*amount = 0;
		return 0;
	}
	if (off + count > s->size) {
		count = s->size - off;
	}
	return oskit_blkio_read(s->io, buf, s->offset + off, count, amount);
}

/*
 * Write data to the block I/O subset object,
 * clipping accesses to the range of this subset.
 */
static OSKIT_COMDECL
write(oskit_blkio_t *io, const void *buf,
      oskit_off_t off, oskit_size_t count, oskit_size_t *amount)
{
	struct sub *s = (struct sub*)io;

	if (off >= s->size) {
		*amount = 0;
		return 0;
	}
	if (off + count > s->size) {
		count = s->size - off;
	}
	return oskit_blkio_write(s->io, buf, s->offset + off, count, amount);
}

/*
 * Return the size of this subset I/O object.
 */
static OSKIT_COMDECL
getsize(oskit_blkio_t *io, oskit_off_t *out_size)
{
	struct sub *s = (struct sub*)io;

	*out_size = s->size;
	return 0;
}

/*
 * Attempt to set the size of this object,
 * which never succeeds in our case.
 */
static OSKIT_COMDECL
setsize(oskit_blkio_t *io, oskit_off_t new_size)
{
	return OSKIT_E_NOTIMPL;
}

/*
 * Operations vector for the subset block I/O interface.
 */
static struct oskit_blkio_ops ops = {
	query, 
	addref, 
	release,
	getblocksize, 
	read, 
	write,
	getsize, 
	setsize
};

oskit_error_t oskit_blkio_subset(oskit_blkio_t *io, oskit_off_t offset,
			  oskit_off_t size, oskit_blkio_t **out_io)
{
	struct sub *s;
	unsigned blksize;

	/* Sanity-check the offset and size */
	blksize = oskit_blkio_getblocksize(io);
	if ((offset & (blksize-1)) || (size & (blksize-1)))
		return OSKIT_E_INVALIDARG;

	/* Create the new object */
	s = malloc(sizeof(*s));
	if (s == NULL)
		return OSKIT_E_OUTOFMEMORY;
	s->ioi.ops = &ops;
	s->refs = 1;
	s->io = io;	
	oskit_blkio_addref(io);
	s->offset = offset;
	s->size = size;
	*out_io = &s->ioi;
	return 0;
}

