/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * Default implementation of buffer routines.
 * Besides providing a useful service,
 * this module serves as an example of how to implement bufio objects.
 */

#include <oskit/io/bufio.h>
#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/assert.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_mem.h>

struct mybufio {
	oskit_bufio_t	ioi;		/* COM I/O Interface */
	unsigned 	count;		/* reference count */
	oskit_size_t	size;		/* size of buffer */
	/* the buffer itself follows... */
};
typedef struct mybufio mybufio_t;

#ifdef INDIRECT_OSENV
static oskit_osenv_mem_t *bufio_mem;
#endif

/* 
 * Query a buffer I/O object for it's interfaces.
 *
 */
static OSKIT_COMDECL
bufio_query(oskit_bufio_t *io, const oskit_iid_t *iid, void **out_ihandle) 
{
	struct mybufio *b;

	b = (mybufio_t *)io;
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
	struct mybufio *b;

	b = (struct mybufio *)io;
	assert(b != NULL);
	assert(b->count != 0);

	return ++b->count;
}

static OSKIT_COMDECL_U
bufio_release(oskit_bufio_t *io)
{
	struct mybufio *b;
	unsigned newcount;

	b = (struct mybufio *)io;

	assert(b != NULL);
	assert(b->count != 0);

	if ((newcount = --b->count) == 0) {
#ifndef INDIRECT_OSENV
		osenv_mem_free(b, OSENV_PHYS_WIRED | OSENV_AUTO_SIZE, 0);
#else
		assert(bufio_mem != 0);

		oskit_osenv_mem_free(bufio_mem,
				     b, OSENV_PHYS_WIRED | OSENV_AUTO_SIZE, 0);
#endif
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
bufio_read(oskit_bufio_t *io, void *dest, oskit_off_t offset, oskit_size_t count,
	   oskit_size_t *out_actual)
{
	struct mybufio *b;

	b = (struct mybufio *)io;

	assert(b != NULL);
	assert(b->count != 0);

	if (offset >= b->size)
		return OSKIT_EINVAL;
	if (offset + count > b->size)
		count = b->size - offset;

	memcpy(dest, (void *)(b+1) + offset, count);

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
	struct mybufio *b;

	b = (struct mybufio *)io;

	assert(b != NULL);
	assert(b->count != 0);

	if (offset >= b->size)
		return OSKIT_EINVAL;
	if (offset + count > b->size)
		count = b->size - offset;

	memcpy((void *)(b+1) + offset, src, count);

	*out_actual = count;
	return 0;
}

/*
 * Return the size of this buffer.
 */
static OSKIT_COMDECL
bufio_getsize(oskit_bufio_t *io, oskit_off_t *out_size)
{
	struct mybufio *b;

	b = (struct mybufio *)io;
	assert(b != NULL);
	assert(b->count != 0);

	*out_size = b->size;
	return 0;
}

static OSKIT_COMDECL
bufio_setsize(oskit_bufio_t *io, oskit_off_t size)
{
	struct mybufio *b;

	b = (struct mybufio *)io;

	assert(b != NULL);
	assert(b->count != 0);

	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
bufio_map(oskit_bufio_t *io, void **out_addr,
	  oskit_off_t offset, oskit_size_t count)
{
	struct mybufio *b;

	b = (struct mybufio *)io;

	assert(b != NULL);
	assert(b->count != 0);

	if (offset + count > b->size)
		return OSKIT_EINVAL;

	*out_addr = (void*)(b+1) + offset; 

	return 0;
}

/* 
 * XXX These should probably do checking on their inputs so we could 
 * catch bugs in the code that calls them.
 */

static OSKIT_COMDECL
bufio_unmap(oskit_bufio_t *io, void *addr, oskit_off_t offset,
	    oskit_size_t count)
{
	struct mybufio *b;

	b = (struct mybufio *)io;

	assert(b != NULL);
	assert(b->count != 0);

	if (offset + count > b->size)
		return OSKIT_EINVAL;

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
oskit_bufio_create(oskit_size_t size) 
{
	mybufio_t *b;

	/* XXX: must be wired memory for DMA drivers */

#ifndef INDIRECT_OSENV
	b = (struct mybufio *)osenv_mem_alloc(
				sizeof(*b) + size,
				OSENV_PHYS_WIRED | OSENV_AUTO_SIZE, 0);
#else
	/*
	 * If this is the first call,
	 * find the osenv, and then the osenv_mem interface.
	 */
	if (bufio_mem == 0) {
		oskit_osenv_t *osenv;

		oskit_lookup_first(&oskit_osenv_iid, (void **)&osenv);
		assert(osenv);
		oskit_osenv_lookup(osenv, &oskit_osenv_mem_iid,
				   (void **)&bufio_mem);
		assert(bufio_mem);
		oskit_osenv_release(osenv);
	}
	b = (struct mybufio *)oskit_osenv_mem_alloc(bufio_mem,
				sizeof(*b) + size,
				OSENV_PHYS_WIRED | OSENV_AUTO_SIZE, 0);
#endif
	if (b == NULL)
		return NULL;

	b->ioi.ops = &bio_ops;
	b->count = 1;
	b->size = size;
	
	return &b->ioi;
}
