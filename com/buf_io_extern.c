/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * A bufio implementation maintaining an external buffer
 */

#include <oskit/io/bufio.h>
#include <oskit/c/string.h>
#include <oskit/c/malloc.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/assert.h>

struct bufio_extern {
	oskit_bufio_t	ioi;		/* COM I/O Interface */
	unsigned 	count;		/* reference count */
	void		*buf;		/* the buffer */
	oskit_size_t	size;		/* size of buffer */
	void		(*free)(void *cookie, void *buf);	/* free */
	void		*cookie;
};
typedef struct bufio_extern bufio_extern_t;


/* 
 * Query a buffer I/O object for its interfaces.
 */
static OSKIT_COMDECL
bufio_query(oskit_bufio_t *io, const oskit_iid_t *iid, void **out_ihandle) 
{
	struct bufio_extern *b = (struct bufio_extern *)io;

	assert(b != NULL);
	assert(b->count != 0);

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_bufio_iid, sizeof(*iid)) == 0) {
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
bufio_addref(oskit_bufio_t *io)
{
	struct bufio_extern *b = (struct bufio_extern *)io;

	assert(b != NULL);
	assert(b->count != 0);

	return ++b->count;
}

static OSKIT_COMDECL_U
bufio_release(oskit_bufio_t *io)
{
	struct bufio_extern *b = (struct bufio_extern *)io;
	unsigned newcount;

	assert(b != NULL);
	assert(b->count != 0);

	if ((newcount = --b->count) == 0) {
		if (b->free)
			b->free(b->cookie, b->buf);
		sfree(b, sizeof *b);
	}

	return newcount;
}

/*
 * Return the block size of this blkio object - always 1.
 */
static OSKIT_COMDECL_U
bufio_getblocksize(oskit_bufio_t *io)
{
	return 1;
}

/*
 * Copy data from a user buffer into kernel's address space.
 */
static OSKIT_COMDECL
bufio_read(oskit_bufio_t *io, void *dest, oskit_off_t offset, 
	oskit_size_t count, oskit_size_t *out_actual)
{
	struct bufio_extern *b = (struct bufio_extern *)io;

	assert(b != NULL);
	assert(b->count != 0);

	if (offset >= b->size)
		return OSKIT_EINVAL;
	if (offset + count > b->size)
		count = b->size - offset;

	memcpy(dest, b->buf + offset, count);

	*out_actual = count;
	return 0;
}

/*
 * Copy data from kernel address space to a user buffer.
 */
static OSKIT_COMDECL
bufio_write(oskit_bufio_t *io, const void *src, oskit_off_t offset,
	    oskit_size_t count, oskit_size_t *out_actual)
{
	struct bufio_extern *b = (struct bufio_extern *)io;

	assert(b != NULL);
	assert(b->count != 0);

	if (offset >= b->size)
		return OSKIT_EINVAL;
	if (offset + count > b->size)
		count = b->size - offset;

	memcpy(b->buf + offset, src, count);

	*out_actual = count;
	return 0;
}

/*
 * Return the size of this buffer.
 */
static OSKIT_COMDECL
bufio_getsize(oskit_bufio_t *io, oskit_off_t *out_size)
{
	struct bufio_extern *b = (struct bufio_extern *)io;

	assert(b != NULL);
	assert(b->count != 0);

	*out_size = b->size;
	return 0;
}

static OSKIT_COMDECL
bufio_setsize(oskit_bufio_t *io, oskit_off_t size)
{
	struct bufio_extern *b = (struct bufio_extern *)io;

	assert(b != NULL);
	assert(b->count != 0);

	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
bufio_map(oskit_bufio_t *io, void **out_addr,
	  oskit_off_t offset, oskit_size_t count)
{
	struct bufio_extern *b = (struct bufio_extern *)io;

	assert(b != NULL);
	assert(b->count != 0);

	if (offset + count > b->size)
		return OSKIT_EINVAL;

	*out_addr = b->buf + offset; 

	return 0;
}

/* 
 * XXX These should probably do checking on their inputs so we could 
 * catch bugs in the code that calls them.
 */

static OSKIT_COMDECL
bufio_unmap(oskit_bufio_t *io, 
	void *addr, oskit_off_t offset, oskit_size_t count)
{
	return 0;
}


static OSKIT_COMDECL 
bufio_wire(oskit_bufio_t *io, oskit_addr_t *out_physaddr,
	   oskit_off_t offset, oskit_size_t count)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL 
bufio_unwire(oskit_bufio_t *io, oskit_addr_t phys_addr,
	     oskit_off_t offset, oskit_size_t count)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
bufio_copy(oskit_bufio_t *io, oskit_off_t offset, oskit_size_t count,
	   oskit_bufio_t **out_io)
{
	return OSKIT_E_NOTIMPL;
}


static struct oskit_bufio_ops bio_ops = {
	bufio_query, bufio_addref, bufio_release,
	bufio_getblocksize,
	bufio_read, bufio_write,
	bufio_getsize, bufio_setsize,
	bufio_map, bufio_unmap,
	bufio_wire, bufio_unwire,
	bufio_copy
};


oskit_bufio_t *
oskit_create_extern_bufio(void *buf, oskit_size_t size,
	void (*free)(void *cookie, void *buf), void *cookie) 
{
	bufio_extern_t *b;

	b = (struct bufio_extern *)smalloc(sizeof(*b));
	if (b == NULL)
		return NULL;

	b->ioi.ops = &bio_ops;
	b->count = 1;
	b->size = size;
	b->buf = buf;
	b->cookie = cookie;
	b->free = free;
	
	return &b->ioi;
}
