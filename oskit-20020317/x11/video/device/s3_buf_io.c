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
#include <oskit/dev/dev.h>
#include <oskit/io/bufio.h>
#include <oskit/c/assert.h>
#include <oskit/c/stdlib.h>

#include "s3_buf_io.h"

/*
 * skbuff COM interface definitions.
 */

#define GET_S3_FROM_BUF_IO(io, b) { \
        (b) = (s3_bufio_t *)io; \
        if ((b) == NULL) \
                panic("%s:%d: null s3_bufio_t", __FILE__, __LINE__); \
        if ((b)->count == 0) \
                panic("%s:%d: bad count", __FILE__, __LINE__); \
}


/*
 * Query a buffer I/O object for it's interfaces.
 */
static OSKIT_COMDECL
query(oskit_bufio_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
	s3_bufio_t *b;

	GET_S3_FROM_BUF_IO(io, b);
	
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_bufio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = (oskit_bufio_t *)&b->ioi;
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
addref(oskit_bufio_t *io)
{
        s3_bufio_t *b;

	GET_S3_FROM_BUF_IO(io, b);

        return ++b->count;
}	


static OSKIT_COMDECL_U
release(oskit_bufio_t *io)
{
        s3_bufio_t *b;
	unsigned newcount;

	GET_S3_FROM_BUF_IO(io, b);

	if ((newcount = --b->count) == 0) {
                /* ref count = 0, need to free! */
        }

	return newcount;
}

static OSKIT_COMDECL
read(oskit_bufio_t *io, void *dest, oskit_off_t offset, oskit_size_t count, 
     oskit_size_t *out_actual)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
write(oskit_bufio_t *io, const void *src,
                oskit_off_t offset, oskit_size_t count,
                oskit_size_t *out_actual)
{
	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL 
getsize(oskit_bufio_t *io, oskit_off_t *out_size)
{
        s3_bufio_t *b;

        GET_S3_FROM_BUF_IO(io, b);

	*out_size = b->size;

        return 0;
}

static OSKIT_COMDECL
setsize(oskit_bufio_t *io, oskit_off_t new_size)
{
        return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
map(oskit_bufio_t *io, void **out_addr, oskit_off_t offset, oskit_size_t count)
{
        s3_bufio_t *b;

        GET_S3_FROM_BUF_IO(io, b);

        if (offset + count > b->size)
                return OSKIT_EINVAL;

        *out_addr = (void*)b->addr + offset;

        return 0;
}


/*
 * XXX These should probably do checking on their inputs so we could
 * catch bugs in the code that calls them.
 */

static OSKIT_COMDECL
unmap(oskit_bufio_t *io, void *addr, oskit_off_t offset, oskit_size_t count)
{
        return 0;
}

static OSKIT_COMDECL
wire(oskit_bufio_t *io, oskit_addr_t *out_phys_addr,
               oskit_off_t offset, oskit_size_t count)
{
        return OSKIT_E_NOTIMPL; /*XXX*/
}


static OSKIT_COMDECL
unwire(oskit_bufio_t *io, oskit_addr_t phys_addr,
                 oskit_off_t offset, oskit_size_t count)
{
        return OSKIT_E_NOTIMPL; /*XXX*/
}


static OSKIT_COMDECL 
copy(oskit_bufio_t *io,
                                   oskit_off_t offset,
                                   oskit_size_t count,
                                   oskit_bufio_t **out_io)
{
	return OSKIT_E_NOTIMPL;
}



static struct oskit_bufio_ops s3_bufio_ops = {
	query,			
	addref,			
	release,		
        (void *) 0,	
	read,
	write,
	getsize,
	setsize,
	map,
	unmap,
	wire,
	unwire,
	copy,
};


oskit_bufio_t *
s3_bufio_create(oskit_addr_t addr, oskit_size_t size)
{
	s3_bufio_t *b;

	b = (s3_bufio_t *)malloc(sizeof(*b));
	if (b == NULL)
		return NULL;
	
	b->ioi.ops = &s3_bufio_ops;
	b->count = 1;
	b->addr = addr;
	b->size = size;

	return &b->ioi;
}
