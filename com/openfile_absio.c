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
 * Generic oskit_openfile adapter for oskit_file+oskit_absio objects.
 */
#include <oskit/fs/openfile.h>
#include <oskit/io/absio.h>
#include <oskit/io/blkio.h>
#include <oskit/error.h>
#include <malloc.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

struct openfile {
	oskit_openfile_t ofi;
	oskit_u32_t count;
	oskit_file_t *file;	/* underlying file object */
	oskit_absio_t *absio;	/* absio interface of same */
	oskit_oflags_t flags;	/* open flags */
	oskit_off_t pos;		/* file position for stream i/o */
};

#if 0
#define DMARK printf(__FILE__ ":%d: " __FUNCTION__ "\n", __LINE__)
#else
#define DMARK ((void)0)
#endif


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
	oskit_absio_release(of->absio);
	sfree(of, sizeof *of);
	return 0;
}


/*** Operations inherited from oskit_stream interface ***/

static OSKIT_COMDECL
openfile_read(oskit_openfile_t *f, void *buf, oskit_u32_t len,
	      oskit_u32_t *out_actual)
{
	struct openfile *of = (void *) f;
	oskit_error_t rc;
	DMARK;

	if ((of->flags & OSKIT_O_ACCMODE) == OSKIT_O_WRONLY)
		return OSKIT_EBADF;
	DMARK;

	rc = oskit_absio_read (of->absio, buf, of->pos, len, out_actual);
	DMARK;
	if (! rc) {
		of->pos += *out_actual;
		DMARK;
	}
	return rc;
}

static OSKIT_COMDECL
openfile_write(oskit_openfile_t *f, const void *buf,
	       oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct openfile *of = (void *) f;
	oskit_error_t rc;
	oskit_off_t size;
	DMARK;

	if ((of->flags & OSKIT_O_ACCMODE) == OSKIT_O_RDONLY)
		return OSKIT_EBADF;

	DMARK;

	/*
	 * This write may need to extend the file.
	 */
	rc = oskit_absio_getsize(of->absio, &size);
	if (!rc && (of->flags & OSKIT_O_APPEND))
		of->pos = size;
	if (!rc && of->pos + len > size)
		rc = oskit_absio_setsize(of->absio, of->pos + len);
	if (!rc)
		rc = oskit_absio_write(of->absio,
				      buf, of->pos, len, out_actual);
	if (!rc)
		of->pos += *out_actual;

	return rc;
}

static OSKIT_COMDECL
openfile_seek(oskit_openfile_t *f, oskit_s64_t offset,
	      oskit_seek_t whence, oskit_u64_t *out_newpos)
{
	struct openfile *of = (void *) f;
	oskit_error_t rc;
	DMARK;

        switch (whence)
        {
                case OSKIT_SEEK_SET:
			break;
                case OSKIT_SEEK_CUR:
			offset += of->pos;
			break;
                case OSKIT_SEEK_END:
                {
			oskit_off_t size;
			rc = oskit_absio_getsize(of->absio, &size);
			if (rc)
				return rc;
			offset += size;
			break;
		}
	default:
		return	OSKIT_EINVAL;
        }

	if (offset < 0)
		return OSKIT_EINVAL;

	*out_newpos = of->pos = offset;
	return 0;
}

static OSKIT_COMDECL
openfile_setsize(oskit_openfile_t *f, oskit_u64_t new_size)
{
	struct openfile *of = (void *) f;
	DMARK;
	return oskit_absio_setsize(of->absio, new_size);
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
openfile_lock_region(oskit_openfile_t *f, oskit_u64_t offset,
		     oskit_u64_t size, oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_unlock_region(oskit_openfile_t *f, oskit_u64_t offset,
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
	/* implement that - XXX */
	return OSKIT_E_NOTIMPL;
}

/*** Operations specific to oskit_openfile ***/

static OSKIT_COMDECL
openfile_getfile(oskit_openfile_t *f, struct oskit_file **out_file)
{
	struct openfile *of = (void *) f;
	DMARK;

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
    openfile_lock_region,
    openfile_unlock_region,
    openfile_stat,
    openfile_clone,
    openfile_getfile
};

/*
 * Sole user entry point for this module.
 * Create a `oskit_openfile' adapter object wrapped around a `oskit_file'
 * object that supports the `oskit_absio' protocol for random-access i/o.
 */

OSKIT_COMDECL
oskit_soa_openfile_from_absio(oskit_file_t *f,
			     oskit_absio_t *aio,
			     oskit_oflags_t flags,
			     struct oskit_openfile **out_openfile)
{
	struct openfile *of;
	oskit_error_t rc;

	DMARK;
	if (flags &~ (OSKIT_O_ACCMODE|OSKIT_O_APPEND))
		return OSKIT_ENOTSUP;

	DMARK;

	if (! aio) {
		rc = oskit_file_query(f, &oskit_absio_iid, (void **) &aio);
		if (rc)
			rc = oskit_file_query(f, &oskit_blkio_iid,
					     (void **) &aio);
		if (rc)
			return rc == OSKIT_E_NOINTERFACE ? OSKIT_ENOTSUP : rc;
	}
	else
		oskit_absio_addref(aio);

	DMARK;
	of = smalloc (sizeof *of);
	if (! of) {
		oskit_absio_release(aio);
		return OSKIT_ENOMEM;
	}
	of->ofi.ops = &openfile_ops;
	of->count = 1;
	of->file = f;
	oskit_file_addref(f);
	of->absio = aio;
	of->flags = flags;
	of->pos = 0L;
	DMARK;

	*out_openfile = &of->ofi;
	return 0;
}
