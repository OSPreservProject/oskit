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
 * Simple object adapter implementing trivial oskit_file_t
 * objects based on a magic open function that's passed in
 */
#include <oskit/fs/soa.h>
#include <oskit/fs/file.h>
#include <oskit/dev/dev.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <fcntl.h>
#include <string.h>

#ifndef VERBOSITY
#define VERBOSITY 	0
#endif

#if VERBOSITY > 20
#define DMARK printf(__FILE__ ":%d: " __FUNCTION__ "\n", __LINE__)
#else
#define DMARK ((void)0)		/* no-op */
#endif

typedef OSKIT_COMDECL (*open_function_t)
	(oskit_file_t *, void *, oskit_oflags_t, oskit_openfile_t **out);

typedef OSKIT_COMDECL_V (*close_function_t)(void *);

/*
 * Structure describing an universal file adapter object
 */
typedef struct universal_file {
	oskit_file_t 		filei;	/* COM interface */
	oskit_u32_t 		count;  /* reference count */

	open_function_t		open;	/* function called on open */
	close_function_t	close;
	void			*cookie;
	struct oskit_stat	stat;
} universal_file_t;

static OSKIT_COMDECL
universal_query(oskit_file_t *ff, const oskit_iid_t *iid, void **out_ihandle)
{
	struct universal_file *f = (struct universal_file *)ff;

	assert(f->count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_file_iid, sizeof(*iid)) == 0) {
                *out_ihandle = f;
                ++f->count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U 
universal_addref(oskit_file_t *ff)
{
	struct universal_file *f = (struct universal_file *)ff;

	assert (f->count);
	return ++f->count;
}

static OSKIT_COMDECL_U
universal_release(oskit_file_t *ff)
{
	struct universal_file *f = (struct universal_file *)ff;

	assert(f->count);

	if (--f->count)
		return f->count;

	f->close(f->cookie);
	sfree(f, sizeof *f);
	return 0;
}

/*** Operations inherited from oskit_posixio_t ***/

static OSKIT_COMDECL
universal_stat(oskit_file_t *file, struct oskit_stat *st)
{
	struct universal_file *f = (void *) file;
	*st = f->stat;
	return 0;
}

static OSKIT_COMDECL
no_setstat(oskit_file_t *f, oskit_u32_t mask, const struct oskit_stat *stats)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
no_pathconf(oskit_file_t *f, oskit_s32_t option, oskit_s32_t *out_val)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
universal_sync(oskit_file_t *f, oskit_bool_t wait)
{
	return 0;		/* another call back ? */
}

static OSKIT_COMDECL
universal_datasync(oskit_file_t *f, oskit_bool_t wait)
{
	return 0;		/* another call back ? */
}

static OSKIT_COMDECL
universal_access(oskit_file_t *f, oskit_amode_t mask)
{
	/* Sure, access me big boy.  */
	return 0;
}

static OSKIT_COMDECL 
no_readlink(oskit_file_t *f, char *buf, oskit_u32_t len, 
	    oskit_u32_t *out_actual)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
universal_open(oskit_file_t *file, oskit_oflags_t flags, 
	     struct oskit_openfile **out_openfile)
{
	struct universal_file *f = (void *) file;
	return f->open(file, f->cookie, flags, out_openfile);
}

static OSKIT_COMDECL	
universal_getfs(oskit_file_t *f, struct oskit_filesystem **out_fs)
{
	return OSKIT_E_NOTIMPL;	/* XXX implement?  */
}

/*
 * file vtable
 */
static struct oskit_file_ops universal_file_ops = {
	universal_query,
	universal_addref,
	universal_release,
	universal_stat,
	no_setstat,
	no_pathconf,
	universal_sync,
	universal_datasync,
	universal_access,
	no_readlink,
	universal_open,
	universal_getfs
};

OSKIT_COMDECL
oskit_soa_file_universal(
    open_function_t open,
    close_function_t close,
    void *cookie,
    struct oskit_stat *stat,
    oskit_file_t **out_file)
{
	struct universal_file *new;

	new = smalloc(sizeof *new);
	if (! new)
		return OSKIT_ENOMEM;

	new->filei.ops = &universal_file_ops;
	new->count = 1;
	new->open = open;
	new->close = close;
	new->cookie = cookie;
	new->stat = *stat;

	*out_file = &new->filei;
	return 0;
}
