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
 */
#include <oskit/fs/openfile.h>
#include <oskit/com/stream.h>
#include <oskit/error.h>
#include <malloc.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

struct openfile {
	oskit_openfile_t ofi;
	oskit_u32_t count;
	oskit_file_t *file;	/* underlying file object */
	oskit_stream_t *stream;	/* stream interface of same, if any */
	oskit_oflags_t flags;	/* open flags */
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

	if (of->stream)
		return oskit_stream_query(of->stream, iid, out_ihandle);

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
	if (of->stream)
		oskit_stream_release(of->stream);
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

	if (! of->stream)
		return OSKIT_E_NOTIMPL;

	return oskit_stream_read (of->stream, buf, len, out_actual);
}

static OSKIT_COMDECL
openfile_write(oskit_openfile_t *f, const void *buf,
	       oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct openfile *of = (void *) f;

	if ((of->flags & OSKIT_O_ACCMODE) == OSKIT_O_RDONLY)
		return OSKIT_EBADF;

	if (! of->stream)
		return OSKIT_E_NOTIMPL;

	return oskit_stream_write (of->stream, buf, len, out_actual);
}

#define STREAMCALL(call, args...)					      \
	struct openfile *of = (void *) f;				      \
	if (! of->stream)						      \
		return OSKIT_E_NOTIMPL;					      \
	return oskit_stream_##call (of->stream ,##args)

static OSKIT_COMDECL
openfile_seek(oskit_openfile_t *f, oskit_s64_t offset,
	      oskit_seek_t whence, oskit_u64_t *out_newpos)
{
	STREAMCALL(seek, offset, whence, out_newpos);
}

static OSKIT_COMDECL
openfile_setsize(oskit_openfile_t *f, oskit_u64_t new_size)
{
	STREAMCALL(setsize, new_size);
}


static OSKIT_COMDECL
openfile_copy_to(oskit_openfile_t *f, oskit_stream_t *dst, oskit_u64_t size,
		       oskit_u64_t *out_read, oskit_u64_t *out_written)
{
	STREAMCALL(copyto, dst, size, out_read, out_written);
}

static OSKIT_COMDECL
openfile_commit(oskit_openfile_t *f, oskit_u32_t commit_flags)
{
	STREAMCALL(commit, commit_flags);
}

static OSKIT_COMDECL
openfile_revert(oskit_openfile_t *f)
{
	STREAMCALL(revert);
}

static OSKIT_COMDECL
openfile_lockregion(oskit_openfile_t *f, oskit_u64_t offset,
		oskit_u64_t size, oskit_u32_t lock_type)
{
	STREAMCALL(lockregion, offset, size, lock_type);
}

static OSKIT_COMDECL
openfile_unlockregion(oskit_openfile_t *f, oskit_u64_t offset,
		oskit_u64_t size, oskit_u32_t lock_type)
{
	STREAMCALL(unlockregion, offset, size, lock_type);
}

static OSKIT_COMDECL
openfile_stat(oskit_openfile_t *f, oskit_stream_stat_t *out_stat,
		  oskit_u32_t stat_flags)
{
	STREAMCALL(stat, out_stat, stat_flags);

}

static OSKIT_COMDECL
openfile_clone(oskit_openfile_t *f, oskit_openfile_t **out_stream)
{
	STREAMCALL(clone, (oskit_stream_t **) out_stream); /* XXX ? */
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
 * object that optionally supports the `oskit_stream' protocol for i/o;
 * if the given stream pointer is non-null, stream operations go there;
 * otherwise stream operations will fail on the returned `oskit_openfile'.
 */

OSKIT_COMDECL
oskit_soa_openfile_from_stream(oskit_file_t *f,
			      oskit_stream_t *stream,
			      oskit_oflags_t flags,
			      struct oskit_openfile **out_openfile)
{
	struct openfile *of;

	if (flags &~ OSKIT_O_ACCMODE)
		return OSKIT_ENOTSUP;

	of = smalloc (sizeof *of);
	if (! of) {
		if (stream)
			oskit_stream_release(stream);
		return OSKIT_ENOMEM;
	}
	of->ofi.ops = &openfile_ops;
	of->count = 1;
	of->file = f;
	oskit_file_addref(f);
	of->stream = stream;
	if (stream)
		oskit_stream_addref(stream);
	of->flags = flags;

	*out_openfile = &of->ofi;
	return 0;
}
