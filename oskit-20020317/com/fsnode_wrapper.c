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
#include <oskit/fs/soa.h>
#include <oskit/com/stream.h>
#include <oskit/fs/filesystem.h>
#include <oskit/fs/file.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/c/assert.h>
#include <oskit/c/malloc.h>
#include <oskit/queue.h>

static struct oskit_absio_ops file_absio_ops;

/*
 * Wrappers for oskit_file and oskit_dir components.
 */
struct gfile {
	oskit_file_t	filei;		/* COM file interface */
	oskit_absio_t	absioi;		/* COM absolute I/O interface */
	int		count;
	oskit_file_t	*w_filei;	/* Wrapped COM file interface */
	oskit_absio_t	*w_absioi;	/* Wrapped COM abs I/O interface */
	void		(*before)(void *);
	void		(*after)(void *);
	void		*cookie;
	queue_chain_t	hashchain;
};

struct gdir 
{
	oskit_dir_t	diri;		/* COM directory interface */
	oskit_absio_t	absioi;		/* COM absolute I/O interface */
	int		count;
	oskit_dir_t	*w_diri;	/* Wrapped COM directory interface */
	oskit_absio_t	*w_absioi;	/* Wrapped COM abs I/O interface */
	void		(*before)(void *);
	void		(*after)(void *);
	void		*cookie;
	queue_chain_t	hashchain;
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
 * Hash table. This table ensures that we do not return different adaptor
 * COM objects for the same underlying COM object. Normally not necessary,
 * but it appears that higher level code depends on it for dir/files.
 * eg: See the mountpoint code in the Posix library
 */
#define NHEADERS	128	/* Must be a power of two! */
static queue_head_t	fsnode_headers[NHEADERS];
static int		fsnode_headers_initialized;

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

static inline int
fsnode_search_htable_file(oskit_file_t *f, struct gfile **out_gfile)
{
	int		index = HASHINDEX((unsigned int) f);
	struct gfile	*file;

	queue_iterate(&fsnode_headers[index], file, struct gfile *, hashchain) {
		if (file->w_filei == f) {
			*out_gfile = file;
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

/*
 * oskit_file methods
 */
static OSKIT_COMDECL
file_query(oskit_file_t *f,
	   const struct oskit_guid *iid, void **out_ihandle)
{
	struct gfile	*file = (struct gfile *) f;

	if (!file || !file->count)
		return OSKIT_E_INVALIDARG;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_posixio_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_file_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &file->filei;
		file->before(file->cookie);
		++file->count;
		file->after(file->cookie);
		return 0;
	}

	if (memcmp(iid, &oskit_absio_iid, sizeof(*iid)) == 0) {
		oskit_error_t	err;
		
		file->before(file->cookie);
		if (file->w_absioi == 0) {
			err = oskit_file_query(file->w_filei, 
				 &oskit_absio_iid, (void **)&file->w_absioi);

			if (err) {
				file->after(file->cookie);
				*out_ihandle = NULL;
				return err;
			}
		}

		*out_ihandle = &file->absioi;
		++file->count;
		file->after(file->cookie);
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
file_addref(oskit_file_t *f)
{
	struct gfile	*file = (struct gfile *) f;
	unsigned	newcount;

	if (!file || !file->count)
		return OSKIT_E_INVALIDARG;

	file->before(file->cookie);
	newcount = ++file->count;
	file->after(file->cookie);

	return newcount;
}

static OSKIT_COMDECL_U
file_release(oskit_file_t *f)
{
	struct gfile	*file = (struct gfile *) f;
        unsigned	newcount;

	if (!file || !file->count)
		return OSKIT_E_INVALIDARG;

	file->before(file->cookie);
	if ((newcount = --file->count) == 0) {
		assert(file->w_filei);
		fsnode_htable_rem(file, file->w_filei, struct gfile *);
		oskit_file_release(file->w_filei);
		if (file->w_absioi)
			oskit_absio_release(file->w_absioi);
		file->after(file->cookie);
		sfree(file, sizeof(*file));
	}
	else
		file->after(file->cookie);
		
	return newcount;
}
	
static OSKIT_COMDECL
file_stat(oskit_file_t *f, struct oskit_stat *out_stats)
{
	struct gfile	*file = (struct gfile *) f;
	oskit_error_t	 err;

	if (!file || !file->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(file, filei, file, stat, out_stats);

	return err;
}

static OSKIT_COMDECL
file_setstat(oskit_file_t *f, oskit_u32_t mask, const struct oskit_stat *stats)
{
	struct gfile	*file = (struct gfile *) f;
	oskit_error_t	 err;

	if (!file || !file->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(file, filei, file, setstat, mask, stats);

	return err;
}

static OSKIT_COMDECL
file_pathconf(oskit_file_t *f, oskit_s32_t option, oskit_s32_t *out_val)
{
	struct gfile	*file = (struct gfile *) f;
	oskit_error_t	 err;

	if (!file || !file->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(file, filei, file, pathconf, option, out_val);

	return err;
}

static OSKIT_COMDECL
file_sync(oskit_file_t *f, oskit_bool_t wait)
{
	struct gfile	*file = (struct gfile *) f;
	oskit_error_t	 err;

	if (!file || !file->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(file, filei, file, sync, wait);

	return err;
}

static OSKIT_COMDECL
file_access(oskit_file_t *f, oskit_amode_t mask)
{
	struct gfile	*file = (struct gfile *) f;
	oskit_error_t	 err;

	if (!file || !file->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(file, filei, file, access, mask);

	return err;
}

static OSKIT_COMDECL
file_readlink(oskit_file_t *f,
	      char *buf, oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct gfile	*file = (struct gfile *) f;
	oskit_error_t	 err;

	if (!file || !file->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(file, filei, file, readlink, buf, len, out_actual);

	return err;
}

static OSKIT_COMDECL
file_open(oskit_file_t *f,
	  oskit_oflags_t iflags, struct oskit_openfile **out_openfile)
{
	struct gfile	 *file = (struct gfile *) f;
	oskit_openfile_t *newofile;
	oskit_error_t	 err;

	if (!file || !file->count)
		return OSKIT_E_INVALIDARG;
	
	err = WRAPPERCALL(file, filei, file, open, iflags, &newofile);

	if (err == 0) {
		/*
		 * The object might not implement per-open state. If not,
		 * then create an "openfile" adaptor from the given
		 * oskit_file_t (the unwrapped one), and then wrap that
		 * new adaptor up before returning it.
		 */
		if (!newofile) {
			err = oskit_soa_openfile_from_file(file->w_filei,
				iflags & (OSKIT_O_RDWR | OSKIT_O_APPEND),
				&newofile);

			if (err)
				return err;
		}

		err = oskit_wrap_openfile_with_file(newofile, f,
			file->before, file->after, file->cookie,
			out_openfile);

		/* wrapper has a reference */
		if (err == 0)
			oskit_openfile_release(newofile);
	}

	return err;
}

static OSKIT_COMDECL
file_getfs(oskit_file_t *f, struct oskit_filesystem **out_fs)
{
	struct gfile	   *file = (struct gfile *) f;
	oskit_filesystem_t *newfs;
	oskit_error_t	   err;

	if (!file || !file->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(file, filei, file, getfs, &newfs);

	if (err == 0) {
		err = oskit_wrap_filesystem(newfs,
			file->before, file->after, file->cookie, out_fs);

		/* wrapper has a reference */
		if (err == 0)
			oskit_filesystem_release(newfs);
	}

	return err;
}

static struct oskit_file_ops file_ops = {
	file_query,
	file_addref,
	file_release,
	file_stat,
	file_setstat,
	file_pathconf,
	file_sync,
	file_sync,
	file_access,
	file_readlink,
	file_open,
	file_getfs
};

/*
 * methods for absio interface of files
 */
static OSKIT_COMDECL
afile_query(oskit_absio_t *io,
	    const struct oskit_guid *iid, void **out_ihandle)
{
	struct gfile	*file = (struct gfile *) (io - 1);

	if (!io || io->ops != &file_absio_ops || !file->count)
		return OSKIT_E_INVALIDARG;

	/* may be file_query or dir_query */
	return file_query(&file->filei, iid, out_ihandle);
}

static OSKIT_COMDECL_U
afile_addref(oskit_absio_t *io)
{
	struct gfile	*file = (struct gfile *) (io - 1);

	if (!io || io->ops != &file_absio_ops || !file->count)
		return OSKIT_E_INVALIDARG;

	return file_addref(&file->filei);
}

static OSKIT_COMDECL_U
afile_release(oskit_absio_t *io)
{
	struct gfile	*file = (struct gfile *) (io - 1);

	if (!io || io->ops != &file_absio_ops || !file->count)
		return OSKIT_E_INVALIDARG;

	/* may be file_release or dir_release */
	return file_release(&file->filei);
}

static OSKIT_COMDECL
afile_read(oskit_absio_t *io,
	   void *buf, oskit_off_t offset, oskit_size_t amount,
	   oskit_size_t *out_actual)
{
	struct gfile	*file = (struct gfile *) (io - 1);
	oskit_error_t	 err;

	if (!io || io->ops != &file_absio_ops || !file->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(file, absioi, absio,
			  read, buf, offset, amount, out_actual);

	return err;
}

static OSKIT_COMDECL
afile_write(oskit_absio_t *io,
	    const void *buf, oskit_off_t offset, oskit_size_t amount,
	    oskit_size_t *out_actual)
{
	struct gfile	*file = (struct gfile *) (io - 1);
	oskit_error_t	 err;

	if (!io || io->ops != &file_absio_ops || !file->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(file, absioi, absio,
			  write, buf, offset, amount, out_actual);

	return err;
}

static OSKIT_COMDECL
afile_getsize(oskit_absio_t *io, oskit_off_t *out_size)
{
	struct gfile	*file = (struct gfile *) (io - 1);
	oskit_error_t	 err;

	if (!io || io->ops != &file_absio_ops || !file->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(file, absioi, absio, getsize, out_size);

	return err;
}

static OSKIT_COMDECL
afile_setsize(oskit_absio_t *io, oskit_off_t new_size)
{
	struct gfile	*file = (struct gfile *) (io - 1);
	oskit_error_t	 err;

	if (!io || io->ops != &file_absio_ops || !file->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(file, absioi, absio, setsize, new_size);

	return err;
}

static struct oskit_absio_ops file_absio_ops = {
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
 * oskit_file wrapper constructor.
 */
oskit_error_t
oskit_wrap_file(oskit_file_t *in,
		void (*before)(),
		void (*after)(),
		void *cookie,
		oskit_file_t **out)
{
        struct gfile	*newfile;

	before(cookie);
	if (!fsnode_headers_initialized)
		fsnode_headers_init();

	/*
	 * Search hash table first to avoid creating multiple adaptors.
	 */
	if (fsnode_search_htable_file(in, &newfile)) {
		newfile->count++;
		*out = &newfile->filei;
		after(cookie);
		return 0;
	}

	newfile = smalloc(sizeof(*newfile));
        if (newfile == NULL)
                return OSKIT_ENOMEM;
        memset(newfile, 0, sizeof(*newfile));

        newfile->count      = 1;
        newfile->filei.ops  = &file_ops;
	newfile->absioi.ops = &file_absio_ops;
 	newfile->w_filei    = in;
	oskit_file_addref(in);

	newfile->before = before;
	newfile->after  = after;
	newfile->cookie = cookie;

	/*
	 * Add to the hash table.
	 */
	fsnode_htable_add(newfile, in, struct gfile *);
	after(cookie);
	
	*out = &newfile->filei;
	return 0;
}

/*
 * oskit_dir methods; we inherit all the file methods above
 */
static OSKIT_COMDECL
dir_query(oskit_dir_t *d, const struct oskit_guid *iid, void **out_ihandle)
{
	struct gdir	*dir = (struct gdir *) d;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	if (memcmp(iid, &oskit_dir_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &dir->diri;
		dir->before(dir->cookie);
		++dir->count;
		dir->after(dir->cookie);
		return 0;
	}

	return file_query((oskit_file_t *) d, iid, out_ihandle);
}

static OSKIT_COMDECL_U
dir_release(oskit_dir_t *d)
{
	struct gdir	*dir = (struct gdir *) d;
        unsigned	newcount;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	dir->before(dir->cookie);
	if ((newcount = --dir->count) == 0) {
		assert(dir->w_diri);
		fsnode_htable_rem(dir, dir->w_diri, struct gdir *);
		oskit_dir_release(dir->w_diri);
		if (dir->w_absioi)
			oskit_absio_release(dir->w_absioi);
		dir->after(dir->cookie);	
		sfree(dir, sizeof(*dir));
	}
	else
		dir->after(dir->cookie);	

	return newcount;
}

static OSKIT_COMDECL
dir_lookup(oskit_dir_t *d, const char *name, oskit_file_t **out_file)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_file_t	*newfile;
	oskit_dir_t	*newdir;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(dir, diri, dir, lookup, name, &newfile);

	if (err == 0) {
		/*
		 * See if a file or a directory came back.
		 */
		err = oskit_file_query(newfile,
				       &oskit_dir_iid, (void **)&newdir);

		if (err) {
			err = oskit_wrap_file(newfile,
				      dir->before, dir->after, dir->cookie,
				      out_file);

			/* wrapper has a reference */
			if (err == 0)
				oskit_file_release(newfile);
		}
		else {
			oskit_dir_release(newdir);
			
			err = oskit_wrap_dir(newdir,
				      dir->before, dir->after, dir->cookie,
				      (oskit_dir_t **) out_file);

			/* wrapper has a reference */
			if (err == 0)
				oskit_dir_release(newdir);
		}
	}
	return err;
}

static OSKIT_COMDECL
dir_create(oskit_dir_t *d,
	   const char *name, oskit_bool_t excl, oskit_mode_t mode,
	   oskit_file_t **out_file)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_file_t	*newfile;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(dir, diri, dir, create, name, excl, mode, &newfile);

	if (err == 0) {
		err = oskit_wrap_file(newfile,
				      dir->before, dir->after, dir->cookie,
				      out_file);

		/* wrapper has a reference */
		if (err == 0)
			oskit_file_release(newfile);
	}
	return err;
}

static OSKIT_COMDECL
dir_link(oskit_dir_t *d, const char *name, oskit_file_t *f)
{
	struct gdir	*dir = (struct gdir *) d;
	struct gfile	*file = (struct gfile *) f;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	assert(file);

	/*
	 * Unwrap the file too! 
	 */
	err = WRAPPERCALL(dir, diri, dir, link, name, file->w_filei);

	return err;
}

static OSKIT_COMDECL
dir_unlink(oskit_dir_t *d, const char *name)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(dir, diri, dir, unlink, name);

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

	/*
	 * Unwrap the second directory too!
	 */
	err = WRAPPERCALL(dir, diri, dir, rename, old_name,
			  new_dir->w_diri, new_name);

	return err;
}

static OSKIT_COMDECL
dir_mkdir(oskit_dir_t *d, const char *name, oskit_mode_t mode)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(dir, diri, dir, mkdir, name, mode);

	return err;
}

static OSKIT_COMDECL
dir_rmdir(oskit_dir_t *d, const char *name)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(dir, diri, dir, rmdir, name);

	return err;
}

static OSKIT_COMDECL
dir_getdirentries(oskit_dir_t *d,
		  oskit_u32_t *inout_ofs, oskit_u32_t nentries,
		  struct oskit_dirents **out_dirents)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_dirents_t *newdirents;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(dir, diri, dir, getdirentries,
			  inout_ofs, nentries, &newdirents);
	if (err == 0) {
		err = oskit_wrap_dirents(newdirents,
			dir->before, dir->after, dir->cookie, out_dirents);

		/* wrapper has a reference */
		if (err == 0)
			oskit_dirents_release(newdirents);
	}

	return err;
}

static OSKIT_COMDECL
dir_mknod(oskit_dir_t *d, const char *name, oskit_mode_t mode, oskit_dev_t dev)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(dir, diri, dir, mknod, name, mode, dev);

	return err;
}

static OSKIT_COMDECL
dir_symlink(oskit_dir_t *d, const char *link_name, const char *dest_name)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(dir, diri, dir, symlink, link_name, dest_name);

	return err;
}

static OSKIT_COMDECL
dir_reparent(oskit_dir_t *d, oskit_dir_t *parent, oskit_dir_t **out_dir)
{
	struct gdir	*dir = (struct gdir *) d;
	oskit_dir_t	*newdir;
	oskit_error_t	err;

	if (!dir || !dir->count)
		return OSKIT_E_INVALIDARG;

	err = WRAPPERCALL(dir, diri, dir, reparent, parent, &newdir);

	if (err == 0) {
		err = oskit_wrap_dir(newdir,
				     dir->before, dir->after, dir->cookie,
				     out_dir);
		/* wrapper has a reference */
		if (err == 0)
			oskit_dir_release(newdir);
	}

	return err;
}

static struct oskit_dir_ops dir_ops = {
	dir_query,
	file_addref,
	dir_release,
	file_stat,
	file_setstat,
	file_pathconf,
	file_sync,
	file_sync,
	file_access,
	file_readlink,
	file_open,
	file_getfs,
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
 * oskit_file wrapper constructor.
 */
oskit_error_t
oskit_wrap_dir(oskit_dir_t *in,
		void (*before)(),
		void (*after)(),
		void *cookie,
		oskit_dir_t **out)
{
        struct gdir	*newdir;

	before(cookie);
	if (!fsnode_headers_initialized)
		fsnode_headers_init();

	/*
	 * Search hash table first to avoid creating multiple adaptors.
	 */
	if (fsnode_search_htable_dir(in, &newdir)) {
		newdir->count++;
		*out = &newdir->diri;
		after(cookie);
		return 0;
	}

	newdir = smalloc(sizeof(*newdir));
        if (newdir == NULL)
                return OSKIT_ENOMEM;
        memset(newdir, 0, sizeof(*newdir));

        newdir->count      = 1;
        newdir->diri.ops   = &dir_ops;
	newdir->absioi.ops = &file_absio_ops;
 	newdir->w_diri     = in;
	oskit_dir_addref(in);

	newdir->before = before;
	newdir->after  = after;
	newdir->cookie = cookie;

	/*
	 * Add to the hash table.
	 */
	fsnode_htable_add(newdir, in, struct gdir *);
	after(cookie);
	
	*out = &newdir->diri;
	return 0;
}
