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
 * objects based on oskit_ttydev_t device objects.
 */
#include <oskit/fs/soa.h>
#include <oskit/fs/file.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/tty.h>
#include <oskit/io/ttystream.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <fcntl.h>
#include <string.h>

#ifndef VERBOSITY
#define VERBOSITY 	100
#endif

#if VERBOSITY > 20
#define DMARK printf(__FILE__ ":%d: " __FUNCTION__ "\n", __LINE__)
#else
#define DMARK ((void)0)		/* no-op */
#endif


/*
 * Structure describing a file adapter object for a device.
 */
struct ttyfile {
	oskit_file_t filei;
	oskit_u32_t count;

	oskit_ttydev_t *dev;
};


static OSKIT_COMDECL
ttyfile_query(oskit_file_t *ff, const oskit_iid_t *iid, void **out_ihandle)
{
	struct ttyfile *f = (struct ttyfile *)ff;

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
ttyfile_addref(oskit_file_t *ff)
{
	struct ttyfile *f = (struct ttyfile *)ff;

	assert (f->count);
	return ++f->count;
}


static OSKIT_COMDECL_U
ttyfile_release(oskit_file_t *ff)
{
	struct ttyfile *f = (struct ttyfile *)ff;

	assert(f->count);

	if (--f->count)
		return f->count;

	oskit_ttydev_release(f->dev);
	sfree(f, sizeof *f);
	return 0;
}


/*** Operations inherited from oskit_posixio_t ***/

static OSKIT_COMDECL
ttyfile_stat(oskit_file_t *file, struct oskit_stat *st)
{
	struct ttyfile *f = (void *) file;

	memset(st, 0, sizeof *st);
	st->ino = (oskit_ino_t) f->dev;
        st->mode = OSKIT_S_IFCHR | OSKIT_S_IRWXG | OSKIT_S_IRWXU | OSKIT_S_IRWXO;
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
ttyfile_sync(oskit_file_t *f, oskit_bool_t wait)
{
	return 0;		/* Data is sunk as far as it goes.  */
}

static OSKIT_COMDECL
ttyfile_datasync(oskit_file_t *f, oskit_bool_t wait)
{
	return 0;		/* Data is sunk as far as it goes.  */
}

static OSKIT_COMDECL
ttyfile_access(oskit_file_t *f, oskit_amode_t mask)
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
ttyfile_open(oskit_file_t *file, oskit_oflags_t flags, 
	     struct oskit_openfile **out_openfile)
{
	struct ttyfile *f = (void *) file;
	oskit_ttystream_t *ts;
	oskit_error_t rc;

	rc = oskit_ttydev_open(f->dev, flags, &ts);
	if (rc)
		return rc;

	rc = oskit_soa_openfile_from_stream(&f->filei, (oskit_stream_t *) ts,
					   flags, out_openfile);
	oskit_ttystream_release(ts);
	return rc;
}

static OSKIT_COMDECL	
ttyfile_getfs(oskit_file_t *f, struct oskit_filesystem **out_fs)
{
	return OSKIT_E_NOTIMPL;	/* XXX implement?  */
}


static struct oskit_file_ops ttyfile_ops = {
	ttyfile_query,
	ttyfile_addref,
	ttyfile_release,
	ttyfile_stat,
	no_setstat,
	no_pathconf,
	ttyfile_sync,
	ttyfile_datasync,
	ttyfile_access,
	no_readlink,
	ttyfile_open,
	ttyfile_getfs
};


OSKIT_COMDECL
oskit_soa_file_from_tty(oskit_ttydev_t *ttydev, oskit_file_t **out_file)
{
	struct ttyfile *new;

	new = smalloc(sizeof *new);
	if (! new)
		return OSKIT_ENOMEM;

	new->filei.ops = &ttyfile_ops;
	new->count = 1;
	new->dev = ttydev;
	oskit_ttydev_addref(ttydev);

	*out_file = &new->filei;
	return 0;
}
