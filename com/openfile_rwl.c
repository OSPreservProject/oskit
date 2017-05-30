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
 * Generic oskit_openfile adapter for oskit_file objects.
 *
 * This is yet another adapter: it simply implements an openfile
 * based on three callback functions the user passes in: one for
 * read, one for write, and one for lseek
 */
#include <oskit/fs/openfile.h>
#include <oskit/fs/soa.h>
#include <oskit/com/stream.h>
#include <oskit/error.h>
#include <malloc.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

typedef OSKIT_COMDECL (*read_function_t)(void *cookie,
		void *buf, oskit_u32_t len, oskit_u32_t *out_actual);

typedef OSKIT_COMDECL (*write_function_t)(void *cookie,
		const void *buf, oskit_u32_t len, oskit_u32_t *out_actual);

typedef OSKIT_COMDECL (*seek_function_t)(void *cookie,
		oskit_s64_t offset, oskit_seek_t whence,
		oskit_u64_t *out_newpos);

struct openfile {
	oskit_openfile_t ofi;
	oskit_u32_t count;
	oskit_file_t *file;	/* underlying file object */
	oskit_oflags_t flags;	/* open flags */
	read_function_t		read;	/* what to do on read  */
	write_function_t	write;  /*               write */
	seek_function_t		seek;	/*               lseek */
	void 		*cookie;
};


static oskit_error_t
openfile_query(oskit_openfile_t *f,
	       const oskit_iid_t *iid, void **out_ihandle)
{
	struct openfile *of = (void *) f;

	assert(of->count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_openfile_iid, sizeof(*iid)) == 0) {
                *out_ihandle = of;
                ++of->count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
openfile_addref(oskit_openfile_t *f)
{
	struct openfile *of = (void *) f;
	assert (of->count);
	return ++of->count;
}

static oskit_u32_t
openfile_release(oskit_openfile_t *f)
{
	struct openfile *of = (void *) f;

	assert(of->count);

	if (--of->count)
		return of->count;

	oskit_file_release(of->file);
	sfree(of, sizeof *of);
	return 0;
}


/*** Operations inherited from oskit_stream interface ***/

static OSKIT_COMDECL
openfile_read(oskit_openfile_t *f, void *buf, oskit_u32_t len,
		  oskit_u32_t *out_actual)
{
	struct openfile *of = (void *) f;

	if ((of->flags & OSKIT_O_ACCMODE) == OSKIT_O_WRONLY)
		return OSKIT_EBADF;

	return of->read ? of->read (of->cookie, buf, len, out_actual)
		: OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_write(oskit_openfile_t *f, const void *buf,
	       oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct openfile *of = (void *) f;

	if ((of->flags & OSKIT_O_ACCMODE) == OSKIT_O_RDONLY)
		return OSKIT_EBADF;

	return of->write ? of->write (of->cookie, buf, len, out_actual)
		: OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_seek(oskit_openfile_t *f, oskit_s64_t offset,
	      oskit_seek_t whence, oskit_u64_t *out_newpos)
{
	struct openfile *of = (void *) f;

	return of->seek ? of->seek (of->cookie, offset, whence, out_newpos)
		: OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_setsize(oskit_openfile_t *f, oskit_u64_t new_size)
{
	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
openfile_copy_to(oskit_openfile_t *f, oskit_stream_t *dst, oskit_u64_t size,
		       oskit_u64_t *out_read, oskit_u64_t *out_written)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_commit(oskit_openfile_t *f, oskit_u32_t commit_flags)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_revert(oskit_openfile_t *f)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_lockregion(oskit_openfile_t *f, oskit_u64_t offset,
		oskit_u64_t size, oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_unlockregion(oskit_openfile_t *f, oskit_u64_t offset,
		oskit_u64_t size, oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_stat(oskit_openfile_t *f, oskit_stream_stat_t *out_stat,
		  oskit_u32_t stat_flags)
{
	return OSKIT_E_NOTIMPL;

}

static OSKIT_COMDECL
openfile_clone(oskit_openfile_t *f, oskit_openfile_t **out_stream)
{
	return OSKIT_E_NOTIMPL;
}

/*** Operations specific to oskit_openfile ***/

static OSKIT_COMDECL
openfile_getfile(oskit_openfile_t *f, struct oskit_file **out_file)
{
	struct openfile *of = (void *) f;

	oskit_file_addref(of->file);
	*out_file = of->file;
	return 0;
}

static struct oskit_openfile_ops openfile_ops = {
    openfile_query,
    openfile_addref,
    openfile_release,
    openfile_read,
    openfile_write,
    openfile_seek,
    openfile_setsize,
    openfile_copy_to,
    openfile_commit,
    openfile_revert,
    openfile_lockregion,
    openfile_unlockregion,
    openfile_stat,
    openfile_clone,
    openfile_getfile
};

/*
 * Sole user entry point for this module.
 * Create a `oskit_openfile' adapter object wrapped around a `oskit_file'
 * object that calls out to callbacks on reads, writes, and lseeks.
 */

OSKIT_COMDECL
oskit_soa_openfile_from_rwl(
	oskit_file_t 		*f,
	oskit_oflags_t 		flags,
	read_function_t		read,
	write_function_t	write,
	seek_function_t		seek,
	void 			*cookie,
	struct oskit_openfile **out_openfile)
{
	struct openfile *of;

	if (flags &~ OSKIT_O_ACCMODE)
		return OSKIT_ENOTSUP;

	of = smalloc (sizeof *of);
	if (! of)
		return OSKIT_ENOMEM;

	of->ofi.ops = &openfile_ops;
	of->count = 1;
	of->file = f;
	oskit_file_addref(f);
	of->read = read;
	of->write = write;
	of->seek = seek;
	of->cookie = cookie;
	of->flags = flags;

	*out_openfile = &of->ofi;
	return 0;
}
