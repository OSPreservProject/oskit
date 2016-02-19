/*
 * Copyright (c) 2001 University of Utah and the Flux Group.
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
 * Absio wrapper.
 */

#include <stdlib.h>
#include <string.h>
#include <oskit/com.h>
#include <oskit/com/wrapper.h>
#include <oskit/io/absio.h>

struct gbio {
	oskit_absio_t    absioi;	/* COM absio interface */
	int		 count;
	oskit_absio_t	 *w_absioi;	/* Wrapped COM blk I/O interface */
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
 * oskit_absio methods.
 */
static OSKIT_COMDECL
absio_query(oskit_absio_t *b, const struct oskit_guid *iid, void **out_ihandle)
{
	struct gbio	*bio = (struct gbio *) b;

	if (!bio || !bio->count)
		return OSKIT_E_INVALIDARG;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_absio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &bio->absioi;
		bio->before(bio->cookie);
		++bio->count;
		bio->after(bio->cookie);
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
absio_addref(oskit_absio_t *b)
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
absio_release(oskit_absio_t *b)
{
	struct gbio	*bio = (struct gbio *) b;
	unsigned	newcount;
    
	if (!bio || !bio->count)
		return OSKIT_E_INVALIDARG;

	bio->before(bio->cookie);
	if ((newcount = --bio->count) == 0) {
		if (bio->w_absioi)
			oskit_absio_release(bio->w_absioi);
		
		bio->after(bio->cookie);
		free(bio);
	}
	else
		bio->after(bio->cookie);

	return newcount;
}

#if 0
/*
 * Return the block size of this block I/O object.
 */
static OSKIT_COMDECL_U
absio_getblocksize(oskit_absio_t *b)
{
	struct gbio	*bio = (struct gbio *) b;
	oskit_error_t	err;

	if (!bio || !bio->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL_NOARGS(bio, absioi, absio, getblocksize);

	return err;
}
#endif

/*
 * Read from the block I/O subset object,
 * clipping accesses to the range of this subset.
 */
static OSKIT_COMDECL
absio_read(oskit_absio_t *b, void *buf,
	   oskit_off_t off, oskit_size_t count, oskit_size_t *amount)
{
	struct gbio	*bio = (struct gbio *) b;
	oskit_error_t	err;

	if (!bio || !bio->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(bio, absioi, absio, read, buf, off, count, amount);

	return err;
}

/*
 * Write data to the block I/O subset object,
 * clipping accesses to the range of this subset.
 */
static OSKIT_COMDECL
absio_write(oskit_absio_t *b, const void *buf,
	    oskit_off_t off, oskit_size_t count, oskit_size_t *amount)
{
	struct gbio	*bio = (struct gbio *) b;
	oskit_error_t	err;

	if (!bio || !bio->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(bio, absioi, absio, write, buf, off, count, amount);

	return err;
}

/*
 * Return the size of this subset I/O object.
 */
static OSKIT_COMDECL
absio_getsize(oskit_absio_t *b, oskit_off_t *out_size)
{
	struct gbio	*bio = (struct gbio *) b;
	oskit_error_t	err;

	if (!bio || !bio->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(bio, absioi, absio, getsize, out_size);

	return err;
}

/*
 * Attempt to set the size of this object,
 * which never succeeds in our case.
 */
static OSKIT_COMDECL
absio_setsize(oskit_absio_t *b, oskit_off_t new_size)
{
	return OSKIT_E_NOTIMPL;
}

/*
 * Operations vector for the subset block I/O interface.
 */
static struct oskit_absio_ops absio_ops = {
	absio_query, 
	absio_addref, 
	absio_release,
	0,
	absio_read, 
	absio_write,
	absio_getsize, 
	absio_setsize
};

/*
 * oskit_absio wrapper constructor
 */
oskit_error_t
oskit_wrap_absio(oskit_absio_t *in,
		 void (*before)(),
		 void (*after)(),
		 void *cookie,
		 oskit_absio_t **out)
{
        struct gbio	*newbio = malloc(sizeof(*newbio));

        if (newbio == NULL)
                return OSKIT_ENOMEM;
	
        memset(newbio, 0, sizeof(*newbio));

        newbio->count      = 1;
	newbio->absioi.ops = &absio_ops;
 	newbio->w_absioi   = in;
	oskit_absio_addref(in);

	newbio->before = before;
	newbio->after  = after;
	newbio->cookie = cookie;

	*out = &newbio->absioi;
	return 0;
}
