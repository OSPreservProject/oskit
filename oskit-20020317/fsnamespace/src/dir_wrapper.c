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

/*
 * This wraps up oskit_dir_t when the name cache is in use, and the program
 * is multithreaded.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include "fsn.h"

#if defined(FSCACHE)
#include <oskit/com.h>
#include <oskit/com/wrapper.h>
#include <oskit/boolean.h>
#include <oskit/io/absio.h>
#include <oskit/fs/soa.h>
#include <oskit/com/stream.h>
#include <oskit/fs/filesystem.h>
#include <oskit/fs/file.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/queue.h>

static struct oskit_absio_ops file_absio_ops;

/*
 * Wrapper for oskit_dir component.
 */
struct gdir 
{
	oskit_dir_t	diri;		/* COM directory interface */
	oskit_absio_t	absioi;		/* COM absolute I/O interface */
	int		count;
	oskit_dir_t	*w_diri;	/* Wrapped COM directory interface */
	oskit_absio_t	*w_absioi;	/* Wrapped COM abs I/O interface */
	queue_chain_t	hashchain;
#ifdef  THREAD_SAFE
	pthread_mutex_t mutex;		/* Protect ref counts */
#endif
	struct fsnimpl  *fsnimpl;
};

#ifdef THREAD_SAFE
/*
 * Must protect the reference counts. However, no need to protect access
 * to the wrapped dir since it already protects itself (is wrapped).
 */
#define GDIR_LOCK(g)	pthread_mutex_lock(&((g)->mutex));
#define GDIR_UNLOCK(g)	pthread_mutex_unlock(&((g)->mutex));
#else
#define GDIR_LOCK(g)	
#define GDIR_UNLOCK(g)	
#endif

#define NOWRAPPERCALL(g, ops, iface, call, args...) ({			      \
	oskit_error_t __err;						      \
	__err = oskit_##iface##_##call(g->w_##ops ,##args);        	      \
	DEBUGWRAPPER(iface, call, __err);				      \
	__err; })

/*
 * Hash table. This table ensures that we do not return different adaptor
 * COM objects for the same underlying COM object. Normally not necessary,
 * but it appears that higher level code depends on it for dir/files.
 * eg: See the mountpoint code in the Posix library
 */
#define NHEADERS	128	/* Must be a power of two! */
static queue_head_t	fsnode_headers[NHEADERS];
static int		fsnode_headers_initialized;

#ifdef THREAD_SAFE
/*
 * This global protects the hash table. 
 */
static pthread_mutex_t  fsnode_headers_mutex = PTHREAD_MUTEX_INITIALIZER;
#define HEADER_LOCK()	pthread_mutex_lock(&fsnode_headers_mutex)
#define HEADER_UNLOCK()	pthread_mutex_unlock(&fsnode_headers_mutex)
#else
#define HEADER_LOCK()
#define HEADER_UNLOCK()
#endif

static inline int
HASHINDEX(unsigned int x)
{
	unsigned int    y = (x >> 16) | (x << 16);

	return ((x ^ y) & (NHEADERS - 1));
}

static inline int
fsnode_search_htable_dir(oskit_dir_t *d, struct gdir **out_gdir)
{
	int		index = HASHINDEX((unsigned int) d);
	struct gdir	*dir;

	queue_iterate(&fsnode_headers[index], dir, struct gdir *, hashchain) {
		if (dir->w_diri == d) {
			*out_gdir = dir;
			return 1;
		}
	}
	return 0;
}

#define fsnode_htable_add(obj, hash, type) ({				\
	int	index = HASHINDEX((unsigned int) hash);			\
									\
	queue_enter(&fsnode_headers[index], obj, type, hashchain);	\
})

#define fsnode_htable_rem(obj, hash, type) ({				\
	int	index = HASHINDEX((unsigned int) hash);			\
									\
	queue_remove(&fsnode_headers[index], obj, type, hashchain);	\
})

static void
fsnode_headers_init(void)
{
	int	i;
	
	for (i = 0; i < NHEADERS; i++)
		queue_init(&fsnode_headers[i]);

	fsnode_headers_initialized = 1;
}

static OSKIT_COMDECL
dir_query(oskit_dir_t *d, const struct oskit_guid *iid, void **out_ihandle)
{
	struct gdir	*dir = (struct gdir *) d;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_dir_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_posixio_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_file_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &dir->diri;
		GDIR_LOCK(dir);
		++dir->count;
		GDIR_UNLOCK(dir);
		return 0;
	}

	if (memcmp(iid, &oskit_absio_iid, sizeof(*iid)) == 0) {
		oskit_error_t	err;

		GDIR_LOCK(dir);
		if (dir->w_absioi == 0) {
			err = oskit_dir_query(dir->w_diri, 
				 &oskit_absio_iid, (void **)&dir->w_absioi);

			if (err) {
				GDIR_UNLOCK(dir);
				*out_ihandle = NULL;
				return err;
			}
		}

		*out_ihandle = &dir->absioi;
		++dir->count;
		GDIR_UNLOCK(dir);
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
dir_addref(oskit_dir_t *d)
{
	struct gdir	*dir = (struct gdir *) d;
	unsigned	newcount;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	GDIR_LOCK(dir);
	newcount = ++dir->count;
	GDIR_UNLOCK(dir);

	return newcount;
}

static OSKIT_COMDECL_U
dir_release(oskit_dir_t *d)
{
	struct gdir	*dir = (struct gdir *) d;
        unsigned	newcount;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	GDIR_LOCK(dir);
	if ((newcount = --dir->count) == 0) {
		assert(dir->w_diri);
		oskit_dir_release(dir->w_diri);
		if (dir->w_absioi)
			oskit_absio_release(dir->w_absioi);
		GDIR_UNLOCK(dir);
		HEADER_LOCK();
		fsnode_htable_rem(dir, dir->w_diri, struct gdir *);
		HEADER_UNLOCK();
		sfree(dir, sizeof(*dir));
	}
	else
		GDIR_UNLOCK(dir);

	return newcount;
}

static OSKIT_COMDECL
dir_stat(oskit_dir_t *d, struct oskit_stat *out_stats)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	 err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir, stat, out_stats);

	return err;
}

static OSKIT_COMDECL
dir_setstat(oskit_dir_t *d, oskit_u32_t mask, const struct oskit_stat *stats)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	 err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir, setstat, mask, stats);

	return err;
}

static OSKIT_COMDECL
dir_pathconf(oskit_dir_t *d, oskit_s32_t option, oskit_s32_t *out_val)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	 err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir, pathconf, option, out_val);

	return err;
}

static OSKIT_COMDECL
dir_sync(oskit_dir_t *d, oskit_bool_t wait)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	 err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir, sync, wait);

	return err;
}

static OSKIT_COMDECL
dir_access(oskit_dir_t *d, oskit_amode_t mask)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	 err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir, access, mask);

	return err;
}

static OSKIT_COMDECL
dir_readlink(oskit_dir_t *d,
	     char *buf, oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	 err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir, readlink, buf, len, out_actual);

	return err;
}

static OSKIT_COMDECL
dir_open(oskit_dir_t *d,
	 oskit_oflags_t iflags, struct oskit_openfile **out_openfile)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	 err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;
	
	err = NOWRAPPERCALL(dir, diri, dir, open, iflags, out_openfile);

	return err;
}

static OSKIT_COMDECL
dir_getfs(oskit_dir_t *d, struct oskit_filesystem **out_fs)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir, getfs, out_fs);

	return err;
}

/*
 * Here start the actual directory methods.
 */
static OSKIT_COMDECL
dir_lookup(oskit_dir_t *d, const char *name, oskit_file_t **out_file)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir, lookup, name, out_file);

	return err;
}

static OSKIT_COMDECL
dir_create(oskit_dir_t *d,
	   const char *name, oskit_bool_t excl, oskit_mode_t mode,
	   oskit_file_t **out_file)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir,
			    create, name, excl, mode, out_file);

	return err;
}

static OSKIT_COMDECL
dir_link(oskit_dir_t *d, const char *name, oskit_file_t *file)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	assert(file);

	err = NOWRAPPERCALL(dir, diri, dir, link, name, file);

	return err;
}

static OSKIT_COMDECL
dir_unlink(oskit_dir_t *d, const char *name)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	FSCACHE_LOCK(dir->fsnimpl);

	err = NOWRAPPERCALL(dir, diri, dir, unlink, name);
	if (!err)
		fsn_cache_remove(dir->fsnimpl, dir->w_diri, name);

	FSCACHE_UNLOCK(dir->fsnimpl);

	return err;
}

static OSKIT_COMDECL
dir_rename(oskit_dir_t *d,
	   const char *old_name, oskit_dir_t *new_d, const char *new_name)
{
	struct gdir	*dir = (struct gdir *) d;
	struct gdir	*new_dir = (struct gdir *) new_d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	if (!new_dir || !new_dir->count)
		return OSKIT_E_INVALIDARG;

	FSCACHE_LOCK(dir->fsnimpl);

	/*
	 * Unwrap the second directory too!
	 */
	err = NOWRAPPERCALL(dir, diri, dir, rename, old_name,
			  new_dir->w_diri, new_name);

	if (!err)
		fsn_cache_rename(dir->fsnimpl,
				 dir->w_diri, old_name,
				 new_dir->w_diri, new_name);

	FSCACHE_UNLOCK(dir->fsnimpl);

	return err;
}

static OSKIT_COMDECL
dir_mkdir(oskit_dir_t *d, const char *name, oskit_mode_t mode)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir, mkdir, name, mode);

	return err;
}

static OSKIT_COMDECL
dir_rmdir(oskit_dir_t *d, const char *name)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	FSCACHE_LOCK(dir->fsnimpl);

	err = NOWRAPPERCALL(dir, diri, dir, rmdir, name);
	if (!err)
		fsn_cache_purge(dir->fsnimpl);

	FSCACHE_UNLOCK(dir->fsnimpl);

	return err;
}

static OSKIT_COMDECL
dir_getdirentries(oskit_dir_t *d,
		  oskit_u32_t *inout_ofs, oskit_u32_t nentries,
		  struct oskit_dirents **out_dirents)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir, getdirentries,
			  inout_ofs, nentries, out_dirents);

	return err;
}

static OSKIT_COMDECL
dir_mknod(oskit_dir_t *d, const char *name, oskit_mode_t mode, oskit_dev_t dev)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir, mknod, name, mode, dev);

	return err;
}

static OSKIT_COMDECL
dir_symlink(oskit_dir_t *d, const char *link_name, const char *dest_name)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir, symlink, link_name, dest_name);

	return err;
}

static OSKIT_COMDECL
dir_reparent(oskit_dir_t *d, oskit_dir_t *parent, oskit_dir_t **out_dir)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, diri, dir, reparent, parent, out_dir);

	return err;
}

static struct oskit_dir_ops dir_ops = {
	dir_query,
	dir_addref,
	dir_release,
	dir_stat,
	dir_setstat,
	dir_pathconf,
	dir_sync,
	dir_sync,
	dir_access,
	dir_readlink,
	dir_open,
	dir_getfs,
	dir_lookup,
	dir_create,
	dir_link,
	dir_unlink,
	dir_rename,
	dir_mkdir,
	dir_rmdir,
	dir_getdirentries,
	dir_mknod,
	dir_symlink,
	dir_reparent
};

/*
 * methods for absio interface of directories (as files).
 */
static OSKIT_COMDECL
afile_query(oskit_absio_t *io,
	    const struct oskit_guid *iid, void **out_ihandle)
{
	struct gdir	*dir = (struct gdir *) (io - 1);

	if (!io || io->ops != &file_absio_ops || !dir->count)
		return OSKIT_E_INVALIDARG;

	return dir_query(&dir->diri, iid, out_ihandle);
}

static OSKIT_COMDECL_U
afile_addref(oskit_absio_t *io)
{
	struct gdir	*dir = (struct gdir *) (io - 1);

	if (!io || io->ops != &file_absio_ops || !dir->count)
		return OSKIT_E_INVALIDARG;

	return dir_addref(&dir->diri);
}

static OSKIT_COMDECL_U
afile_release(oskit_absio_t *io)
{
	struct gdir	*dir = (struct gdir *) (io - 1);

	if (!io || io->ops != &file_absio_ops || !dir->count)
		return OSKIT_E_INVALIDARG;

	/* may be file_release or dir_release */
	return dir_release(&dir->diri);
}

static OSKIT_COMDECL
afile_read(oskit_absio_t *io,
	   void *buf, oskit_off_t offset, oskit_size_t amount,
	   oskit_size_t *out_actual)
{
	struct gdir	*dir = (struct gdir *) (io - 1);
	oskit_error_t	 err;

	if (!io || io->ops != &file_absio_ops || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, absioi, absio,
			    read, buf, offset, amount, out_actual);

	return err;
}

static OSKIT_COMDECL
afile_write(oskit_absio_t *io,
	    const void *buf, oskit_off_t offset, oskit_size_t amount,
	    oskit_size_t *out_actual)
{
	struct gdir	*dir = (struct gdir *) (io - 1);
	oskit_error_t	 err;

	if (!io || io->ops != &file_absio_ops || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, absioi, absio,
			    write, buf, offset, amount, out_actual);

	return err;
}

static OSKIT_COMDECL
afile_getsize(oskit_absio_t *io, oskit_off_t *out_size)
{
	struct gdir	*dir = (struct gdir *) (io - 1);
	oskit_error_t	 err;

	if (!io || io->ops != &file_absio_ops || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, absioi, absio, getsize, out_size);

	return err;
}

static OSKIT_COMDECL
afile_setsize(oskit_absio_t *io, oskit_off_t new_size)
{
	struct gdir	*dir = (struct gdir *) (io - 1);
	oskit_error_t	 err;

	if (!io || io->ops != &file_absio_ops || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = NOWRAPPERCALL(dir, absioi, absio, setsize, new_size);

	return err;
}

static struct oskit_absio_ops dir_absio_ops = {
	afile_query,
	afile_addref,
	afile_release,
	(void *)0,			/* slot reserved for getblocksize */
	afile_read,
	afile_write,
	afile_getsize,
	afile_setsize
};


/*
 * oskit_dir wrapper constructor.
 */
oskit_error_t
fsn_wrap_dir(struct fsnimpl *fsnimpl,
	     oskit_dir_t *in, oskit_dir_t **out)
{
        struct gdir	*newdir;

	assert(((struct gdir *)in)->diri.ops != &dir_ops);

	HEADER_LOCK();

	if (!fsnode_headers_initialized)
		fsnode_headers_init();

	/*
	 * Search hash table first to avoid creating multiple adaptors.
	 */
	if (fsnode_search_htable_dir(in, &newdir)) {
		newdir->count++;
		*out = &newdir->diri;
		HEADER_UNLOCK();
		return 0;
	}

	newdir = smalloc(sizeof(*newdir));
        if (newdir == NULL)
                return OSKIT_ENOMEM;
        memset(newdir, 0, sizeof(*newdir));

        newdir->count      = 1;
        newdir->diri.ops   = &dir_ops;
	newdir->absioi.ops = &dir_absio_ops;
 	newdir->w_diri     = in;
	oskit_dir_addref(in);
#ifdef  THREAD_SAFE
	pthread_mutex_init(&(newdir->mutex), &pthread_mutexattr_default);
#endif
	newdir->fsnimpl    = fsnimpl;
	FSLOCK(fsnimpl);
	fsnimpl->count++;
	FSUNLOCK(fsnimpl);

	/*
	 * Add to the hash table.
	 */
	fsnode_htable_add(newdir, in, struct gdir *);
	HEADER_UNLOCK();
	
	*out = &newdir->diri;
	return 0;
}

/*
 * Internally we may need to access the unwrapped directory object instead
 * of the wrapper.
 */
oskit_error_t
fsn_unwrap_dir(oskit_dir_t *in, oskit_dir_t **out)
{
        struct gdir *gd = (struct gdir *)in;

	if (gd->diri.ops != &dir_ops)
		return OSKIT_EINVAL;

	/*
	 * Add a reference for the caller!
	 */
	oskit_dir_addref(gd->w_diri);

	*out = gd->w_diri;
	return 0;
}
#endif
