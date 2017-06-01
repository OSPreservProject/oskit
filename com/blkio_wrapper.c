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
 * Blkio wrapper.
 */

#include <stdlib.h>
#include <string.h>
#include <oskit/com.h>
#include <oskit/com/wrapper.h>
#include <oskit/io/blkio.h>

struct gbio {
	oskit_blkio_t    blkioi;	/* COM blkio interface */
	int		 count;
	oskit_blkio_t	 *w_blkioi;	/* Wrapped COM blk I/O interface */
	void		 (*before)(void *);
	void		 (*after)(void *);
	void		 *cookie;
};

#define WRAPPERCALL(g, ops, iface, call, args...) ({			      \
	oskit_error_t __err;						      \
	g->before(g->cookie);						      \
	__err = oskit_##iface##_##call(g->w_##ops ,##args);        	      \
	DEBUGWRAPPER(iface, call, __err);				      \
	g->after(g->cookie);						      \
	__err; })

#define WRAPPERCALL_NOARGS(g, ops, iface, call) ({			      \
	oskit_error_t __err;						      \
	g->before(g->cookie);						      \
	__err = oskit_##iface##_##call(g->w_##ops);	      	      	      \
	DEBUGWRAPPER(iface, call, __err);				      \
	g->after(g->cookie);						      \
	__err; })


/*
 * oskit_blkio methods.
 */
static OSKIT_COMDECL
blkio_query(oskit_blkio_t *b, const struct oskit_guid *iid, void **out_ihandle)
{
	struct gbio	*bio = (struct gbio *) b;

	if (!bio || !bio->count)
		return OSKIT_E_INVALIDARG;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_blkio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &bio->blkioi;
		bio->before(bio->cookie);
		++bio->count;
		bio->after(bio->cookie);
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
blkio_addref(oskit_blkio_t *b)
{
	struct gbio	*bio = (struct gbio *) b;
	unsigned	newcount;

	if (!bio || !bio->count)
		return OSKIT_E_INVALIDARG;

	bio->before(bio->cookie);
	newcount = ++bio->count;
	bio->after(bio->cookie);

	return newcount;
}

static OSKIT_COMDECL_U
blkio_release(oskit_blkio_t *b)
{
	struct gbio	*bio = (struct gbio *) b;
	unsigned	newcount;
    
	if (!bio || !bio->count)
		return OSKIT_E_INVALIDARG;

	bio->before(bio->cookie);
	if ((newcount = --bio->count) == 0) {
		if (bio->w_blkioi)
			oskit_blkio_release(bio->w_blkioi);
		
		bio->after(bio->cookie);
		free(bio);
	}
	else
		bio->after(bio->cookie);

	return newcount;
}

/*
 * Return the block size of this block I/O object.
 */
static OSKIT_COMDECL_U
blkio_getblocksize(oskit_blkio_t *b)
{
	struct gbio	*bio = (struct gbio *) b;
	oskit_error_t	err;

	if (!bio || !bio->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL_NOARGS(bio, blkioi, blkio, getblocksize);

	return err;
}

/*
 * Read from the block I/O subset object,
 * clipping accesses to the range of this subset.
 */
static OSKIT_COMDECL
blkio_read(oskit_blkio_t *b, void *buf,
	   oskit_off_t off, oskit_size_t count, oskit_size_t *amount)
{
	struct gbio	*bio = (struct gbio *) b;
	oskit_error_t	err;

	if (!bio || !bio->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(bio, blkioi, blkio, read, buf, off, count, amount);

	return err;
}

/*
 * Write data to the block I/O subset object,
 * clipping accesses to the range of this subset.
 */
static OSKIT_COMDECL
blkio_write(oskit_blkio_t *b, const void *buf,
	    oskit_off_t off, oskit_size_t count, oskit_size_t *amount)
{
	struct gbio	*bio = (struct gbio *) b;
	oskit_error_t	err;

	if (!bio || !bio->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(bio, blkioi, blkio, write, buf, off, count, amount);

	return err;
}

/*
 * Return the size of this subset I/O object.
 */
static OSKIT_COMDECL
blkio_getsize(oskit_blkio_t *b, oskit_off_t *out_size)
{
	struct gbio	*bio = (struct gbio *) b;
	oskit_error_t	err;

	if (!bio || !bio->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(bio, blkioi, blkio, getsize, out_size);

	return err;
}

/*
 * Attempt to set the size of this object,
 * which never succeeds in our case.
 */
static OSKIT_COMDECL
blkio_setsize(oskit_blkio_t *b, oskit_off_t new_size)
{
	return OSKIT_E_NOTIMPL;
}

/*
 * Operations vector for the subset block I/O interface.
 */
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

/*
 * oskit_blkio wrapper constructor
 */
oskit_error_t
oskit_wrap_blkio(oskit_blkio_t *in,
		 void (*before)(),
		 void (*after)(),
		 void *cookie,
		 oskit_blkio_t **out)
{
        struct gbio	*newbio = malloc(sizeof(*newbio));

        if (newbio == NULL)
                return OSKIT_ENOMEM;
	
        memset(newbio, 0, sizeof(*newbio));

        newbio->count      = 1;
	newbio->blkioi.ops = &blkio_ops;
 	newbio->w_blkioi   = in;
	oskit_blkio_addref(in);

	newbio->before = before;
	newbio->after  = after;
	newbio->cookie = cookie;

	*out = &newbio->blkioi;
	return 0;
}
