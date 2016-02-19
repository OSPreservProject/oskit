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
#include <oskit/io/absio.h>
#include <oskit/com/stream.h>
#include <oskit/fs/filesystem.h>
#include <oskit/fs/file.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/c/assert.h>
#include <oskit/c/malloc.h>

static struct oskit_absio_ops ofile_absio_ops;

/*
 * Wrappers for oskit_file and oskit_dir components.
 */
struct gopenfile {
	oskit_openfile_t ofilei;	/* COM open file interface */
	oskit_absio_t	 absioi;	/* COM absolute I/O interface */
	int		 count;
	oskit_openfile_t *w_ofilei;	/* Wrapped COM open file interface */
	oskit_absio_t	 *w_absioi;	/* Wrapped COM abs I/O interface */
	oskit_file_t	 *file;		/* COM file interface open file */
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
 * oskit_openfile methods; per-open operations
 */

static OSKIT_COMDECL
ofile_query(oskit_openfile_t *f,
	    const struct oskit_guid *iid, void **out_ihandle)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_openfile_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &ofile->ofilei;
		ofile->before(ofile->cookie);
		++ofile->count;
		ofile->after(ofile->cookie);
		return 0;
	}

	if (memcmp(iid, &oskit_absio_iid, sizeof(*iid)) == 0) {
		oskit_error_t	err;

		ofile->before(ofile->cookie);
		if (ofile->w_absioi == 0) {
			err = oskit_openfile_query(ofile->w_ofilei,
				 &oskit_absio_iid, (void **)&ofile->w_absioi);

			if (err) {
				ofile->after(ofile->cookie);
				*out_ihandle = NULL;
				return err;
			}
		}

		*out_ihandle = &ofile->absioi;
		++ofile->count;
		ofile->after(ofile->cookie);
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
ofile_addref(oskit_openfile_t *f)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	unsigned		newcount;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	ofile->before(ofile->cookie);
	newcount = ++ofile->count;
	ofile->after(ofile->cookie);

	return newcount;
}

static OSKIT_COMDECL_U
ofile_release(oskit_openfile_t *f)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	unsigned		newcount;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	ofile->before(ofile->cookie);
	if ((newcount = --ofile->count) == 0) {
		if (ofile->w_ofilei)
			oskit_openfile_release(ofile->w_ofilei);
		if (ofile->w_absioi)
			oskit_absio_release(ofile->w_absioi);
		ofile->after(ofile->cookie);

		if (ofile->file)
			oskit_file_release(ofile->file);
		free(ofile);
	}
	else
		ofile->after(ofile->cookie);

	return newcount;
}

static OSKIT_COMDECL
ofile_read(oskit_openfile_t *f,
	   void *buf, oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	oskit_error_t		err;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, ofilei, openfile, read, buf, len, out_actual);

	return err;
}

static OSKIT_COMDECL
ofile_write(oskit_openfile_t *f,
	    const void *buf, oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	oskit_error_t		err;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, ofilei, openfile, write, buf, len,out_actual);

	return err;
}

static OSKIT_COMDECL
ofile_seek(oskit_openfile_t *f,
	   oskit_s64_t ofs, oskit_seek_t whence, oskit_u64_t *out_newpos)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	oskit_error_t		err;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, ofilei, openfile,
			  seek, ofs, whence, out_newpos);

	return err;
}

static OSKIT_COMDECL
ofile_setsize(oskit_openfile_t *f, oskit_u64_t new_size)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	oskit_error_t		err;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, ofilei, openfile, setsize, new_size);

	return err;
}

static OSKIT_COMDECL
ofile_copyto(oskit_openfile_t *f,
	      oskit_stream_t *dst, oskit_u64_t size,
	      oskit_u64_t *out_read, oskit_u64_t *out_written)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	oskit_error_t		err;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, ofilei, openfile,
			  copyto, dst, size, out_read, out_written);

	return err;
}

static OSKIT_COMDECL
ofile_commit(oskit_openfile_t *f, oskit_u32_t commit_flags)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	oskit_error_t		err;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, ofilei, openfile, commit, commit_flags);

	return err;
}

static OSKIT_COMDECL
ofile_revert(oskit_openfile_t *f)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	oskit_error_t		err;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL_NOARGS(ofile, ofilei, openfile, revert);

	return err;
}

static OSKIT_COMDECL
ofile_lockregion(oskit_openfile_t *f,
		  oskit_u64_t offset, oskit_u64_t size, oskit_u32_t lock_type)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	oskit_error_t		err;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, ofilei, openfile,
			  lockregion, offset, size, lock_type);

	return err;
}

static OSKIT_COMDECL
ofile_unlockregion(oskit_openfile_t *f,
		    oskit_u64_t offset, oskit_u64_t size,
		    oskit_u32_t lock_type)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	oskit_error_t		err;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, ofilei, openfile,
			  unlockregion, offset, size, lock_type);

	return err;
}

static OSKIT_COMDECL
ofile_stat(oskit_openfile_t *f,
	   oskit_stream_stat_t *out_stat, oskit_u32_t stat_flags)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	oskit_error_t		err;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, ofilei, openfile, stat, out_stat, stat_flags);

	return err;
}

static OSKIT_COMDECL
ofile_clone(oskit_openfile_t *f, oskit_openfile_t **out_stream)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	oskit_error_t		err;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, ofilei, openfile, clone, out_stream);

	return err;
}

static OSKIT_COMDECL
ofile_getfile(oskit_openfile_t *f, struct oskit_file **out_file)
{
	struct gopenfile	*ofile = (struct gopenfile *) f;
	oskit_error_t		err;

	if (!ofile || !ofile->count)
		return OSKIT_E_INVALIDARG;

	if (!ofile->file) {
		oskit_file_t	*newfile;

		err = WRAPPERCALL(ofile, ofilei, openfile, getfile, &newfile);

		if (err)
			return err;

		err = oskit_wrap_file(newfile,
				      ofile->before, ofile->after,
				      ofile->cookie, out_file);

		if (err)
			return err;

		ofile->file = *out_file;
	}
	else
		oskit_file_addref(ofile->file);

	*out_file = ofile->file;
	return 0;
}

static struct oskit_openfile_ops ofile_ops = {
	ofile_query,
	ofile_addref,
	ofile_release,
	ofile_read,
	ofile_write,
	ofile_seek,
	ofile_setsize,
	ofile_copyto,
	ofile_commit,
	ofile_revert,
	ofile_lockregion,
	ofile_unlockregion,
	ofile_stat,
	ofile_clone,
	ofile_getfile
};

/*
 * oskit_openfile absolute I/O methods
 */
static OSKIT_COMDECL
afile_query(oskit_absio_t *io,
	    const struct oskit_guid *iid, void **out_ihandle)
{
	struct gopenfile	*ofile = (struct gopenfile *) (io - 1);

	if (!io || io->ops != &ofile_absio_ops || !ofile->count)
		return OSKIT_E_INVALIDARG;

	return ofile_query(&ofile->ofilei, iid, out_ihandle);
}

static OSKIT_COMDECL_U
afile_addref(oskit_absio_t *io)
{
	struct gopenfile	*ofile = (struct gopenfile *) (io - 1);

	if (!io || io->ops != &ofile_absio_ops || !ofile->count)
		return OSKIT_E_INVALIDARG;

	return ofile_addref(&ofile->ofilei);
}

static OSKIT_COMDECL_U
afile_release(oskit_absio_t *io)
{
	struct gopenfile	*ofile = (struct gopenfile *) (io - 1);

	if (!io || io->ops != &ofile_absio_ops || !ofile->count)
		return OSKIT_E_INVALIDARG;

	return ofile_release(&ofile->ofilei);
}

static OSKIT_COMDECL
afile_read(oskit_absio_t *io, void *buf,
	   oskit_off_t offset, oskit_size_t amount, oskit_size_t *out_actual)
{
	struct gopenfile	*ofile = (struct gopenfile *) (io - 1);
	oskit_error_t		err;

	if (!io || io->ops != &ofile_absio_ops || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, absioi, absio, read,
			  buf, offset, amount, out_actual);

	return err;
}

static OSKIT_COMDECL
afile_write(oskit_absio_t *io,
	    const void *buf, oskit_off_t offset, oskit_size_t amount,
	    oskit_size_t *out_actual)
{
	struct gopenfile	*ofile = (struct gopenfile *) (io - 1);
	oskit_error_t		err;

	if (!io || io->ops != &ofile_absio_ops || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, absioi, absio, write,
			  buf, offset, amount, out_actual);

	return err;
}

static OSKIT_COMDECL
afile_getsize(oskit_absio_t *io, oskit_off_t *out_size)
{
	struct gopenfile	*ofile = (struct gopenfile *) (io - 1);
	oskit_error_t		err;

	if (!io || io->ops != &ofile_absio_ops || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, absioi, absio, getsize, out_size);

	return err;
}

static OSKIT_COMDECL
afile_setsize(oskit_absio_t *io, oskit_off_t new_size)
{
	struct gopenfile	*ofile = (struct gopenfile *) (io - 1);
	oskit_error_t		err;

	if (!io || io->ops != &ofile_absio_ops || !ofile->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(ofile, absioi, absio, setsize, new_size);

	return err;
}

static struct oskit_absio_ops ofile_absio_ops = {
	afile_query,
	afile_addref,
	afile_release,
	(void *)0,
	afile_read,
	afile_write,
	afile_getsize,
	afile_setsize
};

/*
 * oskit_openfile wrapper constructor
 */
oskit_error_t
oskit_wrap_openfile(oskit_openfile_t *in,
		    void (*before)(),
		    void (*after)(),
		    void *cookie,
		    oskit_openfile_t **out)
{
        struct gopenfile	*newofile = malloc(sizeof(*newofile));

        if (newofile == NULL)
                return OSKIT_ENOMEM;
        memset(newofile, 0, sizeof(*newofile));

        newofile->count      = 1;
        newofile->ofilei.ops = &ofile_ops;
	newofile->absioi.ops = &ofile_absio_ops;
 	newofile->w_ofilei   = in;
	oskit_openfile_addref(in);

	newofile->before = before;
	newofile->after  = after;
	newofile->cookie = cookie;

	*out = &newofile->ofilei;
	return 0;
}

/*
 * oskit_openfile wrapper constructor. Internal version that takes
 * the wrapped oskit_file_t that is being opened.
 */
oskit_error_t
oskit_wrap_openfile_with_file(oskit_openfile_t *in,
			      oskit_file_t *infile,
			      void (*before)(),
			      void (*after)(),
			      void *cookie,
			      oskit_openfile_t **out)
{
        struct gopenfile	*newofile = malloc(sizeof(*newofile));

        if (newofile == NULL)
                return OSKIT_ENOMEM;
        memset(newofile, 0, sizeof(*newofile));

        newofile->count      = 1;
        newofile->ofilei.ops = &ofile_ops;
	newofile->absioi.ops = &ofile_absio_ops;
 	newofile->w_ofilei   = in;
 	newofile->file       = infile;
	oskit_openfile_addref(in);
	oskit_file_addref(infile);

	newofile->before = before;
	newofile->after  = after;
	newofile->cookie = cookie;

	*out = &newofile->ofilei;
	return 0;
}
