 /*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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

#include <oskit/com.h>
#include <oskit/com/wrapper.h>
#include <oskit/c/string.h>
#include <oskit/boolean.h>
#include <oskit/fs/dir.h>
#include <oskit/c/assert.h>
#include <oskit/c/malloc.h>

/*
 * Wrappers for oskit_dirents components.
 */
struct gdirents {
	oskit_dirents_t	direntsi;	/* COM dirents interface */
	int		count;
	oskit_dirents_t	*w_direntsi;	/* Wrapped COM dirents interface */
	void		(*before)(void *);
	void		(*after)(void *);
	void		*cookie;
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
 * oskit_dirents methods
 */
static OSKIT_COMDECL
dirents_query(oskit_dirents_t *d,
	      const struct oskit_guid *iid, void **out_ihandle)
{
	struct gdirents	*dirents = (struct gdirents *) d;

	if (!dirents || !dirents->count)
		return OSKIT_E_INVALIDARG;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_dirents_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &dirents->direntsi;
		dirents->before(dirents->cookie);
		++dirents->count;
		dirents->after(dirents->cookie);
		return 0;
	}
	
	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
dirents_addref(oskit_dirents_t *d)
{
	struct gdirents	*dirents = (struct gdirents *) d;
	unsigned	newcount;
	
	if (!dirents || !dirents->count)
		return OSKIT_E_INVALIDARG;

	dirents->before(dirents->cookie);
	newcount = ++dirents->count;
	dirents->after(dirents->cookie);

	return newcount;
}

static OSKIT_COMDECL_U
dirents_release(oskit_dirents_t *d)
{
	struct gdirents	*dirents = (struct gdirents *) d;
	unsigned	newcount;
	
	if (!dirents || !dirents->count)
		return OSKIT_E_INVALIDARG;

	dirents->before(dirents->cookie);
	if ((newcount = --dirents->count) == 0) {
		assert(dirents->w_direntsi);
		oskit_dirents_release(dirents->w_direntsi);
		dirents->after(dirents->cookie);
		sfree(dirents, sizeof(*dirents));
	}
	else
		dirents->after(dirents->cookie);
		
	return newcount;
}

static OSKIT_COMDECL
dirents_getcount(oskit_dirents_t *d, int *out_count)
{
	struct gdirents	*dirents = (struct gdirents *) d;
	oskit_error_t	err;

	if (!dirents || !dirents->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(dirents, direntsi, dirents, getcount, out_count);

	return err;
}

static OSKIT_COMDECL
dirents_getnext(oskit_dirents_t *d, oskit_dirent_t *out_dirent)
{
	struct gdirents	*dirents = (struct gdirents *) d;
	oskit_error_t	err;

	if (!dirents || !dirents->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(dirents, direntsi, dirents, getnext, out_dirent);

	return err;
}

static OSKIT_COMDECL
dirents_rewind(oskit_dirents_t *d)
{
	struct gdirents	*dirents = (struct gdirents *) d;
	oskit_error_t	err;

	if (!dirents || !dirents->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL_NOARGS(dirents, direntsi, dirents, rewind);

	return err;
}

static struct oskit_dirents_ops dirents_ops = {
	dirents_query,
	dirents_addref,
	dirents_release,
	dirents_getcount,
	dirents_getnext,
	dirents_rewind,
};

/*
 * oskit_dirents wrapper constructor.
 */
oskit_error_t
oskit_wrap_dirents(oskit_dirents_t *in,
		   void (*before)(),
		   void (*after)(),
		   void *cookie,
		   oskit_dirents_t **out)
{
        struct gdirents	*newdirents;

	before(cookie);

	newdirents = smalloc(sizeof(*newdirents));
        if (newdirents == NULL)
                return OSKIT_ENOMEM;
        memset(newdirents, 0, sizeof(*newdirents));

        newdirents->count         = 1;
        newdirents->direntsi.ops  = &dirents_ops;
 	newdirents->w_direntsi    = in;
	oskit_dirents_addref(in);

	newdirents->before = before;
	newdirents->after  = after;
	newdirents->cookie = cookie;

	after(cookie);
	
	*out = &newdirents->direntsi;
	return 0;
}
