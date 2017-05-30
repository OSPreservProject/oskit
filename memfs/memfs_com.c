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
 * Implements oskit_file_t, oskit_dir_t, and oskit_openfile_t for the memory
 * filesystem.
 *
 * We only implement the parts that make sense.
 */
#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/fs/filesystem.h>
#include <oskit/dev/dev.h> /* for osenv_ functions */
#include <oskit/io/absio.h>
#include <oskit/io/bufio.h>
#include <oskit/fs/memfs.h>
#include <string.h>
#include <oskit/machine/base_paging.h>
#include <stddef.h>
#include "memfs_osenv.h"

#ifndef VERBOSITY
#define VERBOSITY 	0
#endif

#ifndef KNIT
/*
 * This component requires two log objects:
 * 1) a per-component object
 * 2) a global object used when Bad Things happen.
 * This is the latter - only it should be shared by all osenv users.
 */
oskit_osenv_log_t *memfs_oskit_osenv_log = 0;
#endif

#if VERBOSITY > 20
#define DMARK(fsys) osenv_local_log(fsys, OSENV_LOG_INFO,__FILE__ ":%d: " __FUNCTION__ "\n", __LINE__)
#else
#define DMARK(fsys) ((void)0)		/* no-op */
#endif

/* 
 * A wee note about reference counting.
 *
 * All file objects (struct memfs*) use reference counting to keep
 * track of links in the usual Unix manner.  For this, we use the
 * count_internal field.
 *
 * To avoid confusion (leading to memory leaks) we use a separate
 * reference count to determine when the entire filesystem is
 * unreachable and should be deleted.  This (fs->count_external) keeps
 * track of how many file objects have COM references plus the number
 * of COM references to the filesystem object itself.
 * When this count drops to 0, we zap the whole filesystem.
 *
 * Finally, to handle the special case of releasing a file which has
 * already been deleted, every file object contains the number of COM
 * references (count_external) to that object.  A file object is freed
 * when it has no internal references and non external references.
 *
 * We do not reference count the pointers from file objects to the
 * fsys object but, as a simple way of preventing deletion of the root
 * directory, we do reference count the pointer from the fsys object
 * to the root directory.
 */

/*
 * struct to describe a memory file or directory;
 * the ops vector differentiates.
 */
struct memfs {
	oskit_file_t filei;
	oskit_u32_t count_internal;
	oskit_u32_t count_external;

	struct memfs_fsys *fsys;

	char *name;		/* name of this file */
	oskit_size_t namelen;	/* bytes in name, not including terminator */
        struct memfs *next, **prevp; /* siblings in directory */

	union {
		struct {
			oskit_bufio_t bufioi;
			oskit_size_t size;	/* file size */
			void *data;		/* file data */
			oskit_size_t allocsize;	/* bytes smalloc'd */
			oskit_bool_t can_sfree;	/* data is smalloc'd */
			oskit_bool_t inhibit_resize; /* disallow resizing */
		} file;
		struct {
			oskit_dir_t *parent; /* .. directory, null for self */
			struct memfs *contents; /* files in this directory */
		} dir;
	} data;
};

struct memfs_fsys {
	oskit_filesystem_t fsysi;
	oskit_u32_t count_external;

	oskit_dir_t *root;

#ifndef KNIT
        /*
	 * MEMFS needs these two osenv interfaces. They are provided
	 * to the init routine below, so watch out if you add any. See
	 * memfs_osenv.h for the macros.  
	 */
	oskit_osenv_mem_t *mem;
	oskit_osenv_log_t *log;
#endif
};

/*
 * Internal representation of a single dirent entry.
 */
typedef struct internal_dirent {
	oskit_size_t	namelen;		/* length of name */
	oskit_ino_t	ino;			/* entry inode */
	char	       *name;			/* entry name */
} internal_dirent_t;
static oskit_error_t		fs_dirents_create(struct memfs_fsys *fsys,
					internal_dirent_t *entries,
					int count,
					oskit_dirents_t **out_dirents);

static struct oskit_file_ops memfs_file_ops, memfs_symlink_ops;
static struct oskit_dir_ops memfs_dir_ops;
static struct oskit_bufio_ops memfs_bufio_ops;

static OSKIT_COMDECL_U memfsfilesystem_release(oskit_filesystem_t *b0);
static void            memfs_file_release_internal(struct memfs *b);

static OSKIT_COMDECL
memfs_dir_query(oskit_dir_t *b0, const oskit_iid_t *iid, void **out_ihandle)
{
	struct memfs *b = (struct memfs *)b0;
	osenv_assert(b->count_external);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_posixio_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_file_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_dir_iid, sizeof(*iid)) == 0) {
                *out_ihandle = b;
                ++b->count_external;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL
memfs_file_query(oskit_file_t *b0, const oskit_iid_t *iid, void **out_ihandle)
{
        struct memfs *b = (struct memfs *)b0;
	osenv_assert(b->count_external);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_posixio_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_file_iid, sizeof(*iid)) == 0) {
                *out_ihandle = b;
                ++b->count_external;
                return 0;
        }

	if (memcmp(iid, &oskit_bufio_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_absio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &b->data.file.bufioi;
		++b->count_external;
		return 0;
	}

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
memfs_file_addref(oskit_file_t *b0)
{
        struct memfs *f = (struct memfs *)b0;

	if (f->count_external == 0) {
		++f->fsys->count_external;
	}
	return ++f->count_external;
}

static void
memfs_file_free(struct memfs *b)
{
#ifndef KNIT
	struct memfs_fsys *fsys = b->fsys;
#endif

#if VERBOSITY > 20
	osenv_local_log(fsys, OSENV_LOG_INFO, __FUNCTION__": memfs freeing '%s'\n",
			b->name);
#endif
	osenv_mem_free(fsys, b->name, OSENV_NONBLOCKING, b->namelen + 1);
	if ((void *)b->filei.ops == &memfs_dir_ops) {
		if (b->data.dir.parent)
			memfs_file_release_internal((struct memfs*)b->data.dir.parent);

	} else {
		if (b->data.file.can_sfree)
			osenv_mem_free(b->fsys, b->data.file.data, OSENV_NONBLOCKING,
				       b->data.file.allocsize);
	}
	osenv_mem_free(fsys, b, OSENV_NONBLOCKING, sizeof *b);
}

static OSKIT_COMDECL_U
memfs_file_release(oskit_file_t *b0)
{
        struct memfs *b = (struct memfs *)b0;
	struct memfs_fsys *fsys;

	osenv_assert(b->count_external);
	--b->count_external;

	if (b->count_external)
		return b->count_external;

	fsys = b->fsys;
	if (b->count_internal == 0) {
		/* 
		 * This file has already been unlinked so free it.
		 */
		memfs_file_free(b);
	}
	memfsfilesystem_release((oskit_filesystem_t *)fsys);
	return 0;
}

inline static void 
memfs_addref_internal(struct memfs *f)
{
	f->count_internal++;
}

static void
memfs_file_release_internal(struct memfs *b)
{
	osenv_assert(b->count_internal);

	--b->count_internal;
	if (b->count_internal == 0 && b->count_external == 0) {
		/* 
		 * No external references, so free it now.
		 */
		memfs_file_free(b);
	}
}

static struct memfs *
memfs_file_allocate(struct memfs_fsys *fs,
		    const char *name, void *ops, oskit_dir_t *parent)
{
	struct memfs *new = osenv_mem_alloc(fs, sizeof *new, OSENV_NONBLOCKING, 0);
	if (new) {
		new->namelen = strlen(name);
		new->name = osenv_mem_alloc(fs, new->namelen + 1,
					    OSENV_NONBLOCKING, 0);
		if (!new->name) {
			osenv_mem_free(fs, new, OSENV_NONBLOCKING, sizeof *new);
			return NULL;
		}
		memcpy(new->name, name, new->namelen + 1);

		new->count_internal = 1;
		new->count_external = 0;
		new->filei.ops = ops;
		new->fsys = fs;
		if (ops == &memfs_dir_ops) {
			if (parent) {
				new->data.dir.parent = parent;
				memfs_addref_internal((struct memfs *)parent);
			} else
				new->data.dir.parent = NULL;
			new->data.dir.contents = NULL;
		} else {
			new->data.file.bufioi.ops = &memfs_bufio_ops;
			new->data.file.size = 0;
			new->data.file.allocsize = 0;
			new->data.file.can_sfree = 0;
			new->data.file.inhibit_resize = 0;
		}
	}
	return new;
}



/*** Operations inherited from oskit_posixio_t ***/

static OSKIT_COMDECL
memfsfile_stat(oskit_file_t *f, struct oskit_stat *st)
{
	struct memfs *b = (void *)f;

	memset(st, 0, sizeof *st);
	st->ino = (oskit_ino_t) b;
	st->size = b->data.file.size;
        st->mode = OSKIT_S_IFREG | OSKIT_S_IRWXG | OSKIT_S_IRWXU | OSKIT_S_IRWXO;
	st->nlink = b->count_internal;
	return 0;
}

static OSKIT_COMDECL
memfsdir_stat(oskit_dir_t *f, struct oskit_stat *st)
{
	struct memfs *b = (void *)f;

	memset(st, 0, sizeof *st);
	st->ino = (oskit_ino_t) b;
        st->mode = OSKIT_S_IFDIR | OSKIT_S_IRWXG | OSKIT_S_IRWXU | OSKIT_S_IRWXO;
	st->nlink = b->count_internal + 1; /* Account for self link */
	return 0;
}

static oskit_error_t
memfs_resize(struct memfs *b, oskit_size_t size, oskit_bool_t override_inhibit)
{
	if (size < 0)
		return OSKIT_EINVAL;
	else if (size == b->data.file.size)
		return 0;
	else if (!override_inhibit && b->data.file.inhibit_resize)
		return OSKIT_EPERM;
	else if (size < b->data.file.allocsize) {
		if (size > b->data.file.size)
				/*
				 * Zero-fill the new space.
				 */
			memset((char *)b->data.file.data +
			       b->data.file.size, 0,
			       size - b->data.file.size);
		else if (size == 0 && b->data.file.can_sfree) {
			osenv_mem_free(b->fsys, b->data.file.data, OSENV_NONBLOCKING,
				       b->data.file.allocsize);
			b->data.file.allocsize = 0;
			b->data.file.can_sfree = 0;
		}
		b->data.file.size = size;
	} else {
		/*
		 * Must grow the file.
		 */
		char *new;
		oskit_size_t newsize = round_page(size);
		new = osenv_mem_alloc(b->fsys, newsize, OSENV_NONBLOCKING, PAGE_SIZE);
		if (!new)
			return OSKIT_ENOSPC;
		else {
			memcpy(new, b->data.file.data,
			       b->data.file.size);
			memset(new + b->data.file.size, 0,
			       size - b->data.file.size);
			if (b->data.file.can_sfree)
				osenv_mem_free(b->fsys, b->data.file.data,
					       OSENV_NONBLOCKING,
					       b->data.file.allocsize);
			b->data.file.allocsize = newsize;
			b->data.file.can_sfree = 1;
			b->data.file.size = size;
			b->data.file.data = new;
		}
	}
	return 0;
}

static OSKIT_COMDECL
memfsfile_setstat(oskit_file_t *f, oskit_u32_t mask, const struct oskit_stat *stats)
{
	struct memfs *b = (void *)f;

	if (mask & ~OSKIT_STAT_SIZE)
		return OSKIT_ENOTSUP;
	if (mask & OSKIT_STAT_SIZE) {
		oskit_error_t rc;
		rc = memfs_resize(b, stats->size, 0);
		return rc;
	}
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
memfsfile_sync(oskit_file_t *f, oskit_bool_t wait)
{
	return 0;		/* Data is sunk as far as it goes.  */
}

static OSKIT_COMDECL
memfsfile_datasync(oskit_file_t *f, oskit_bool_t wait)
{
	return 0;		/* Data is sunk as far as it goes.  */
}

static OSKIT_COMDECL
memfsfile_access(oskit_file_t *f, oskit_amode_t mask)
{
	/* Sure, access me big boy.  */
	return 0;
}

static OSKIT_COMDECL
memfsfile_readlink(oskit_file_t *f, char *buf, oskit_u32_t len,
		  oskit_u32_t *out_actual)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
memfsfile_open(oskit_file_t *d, oskit_oflags_t flags,
	     struct oskit_openfile **out_openfile)
{
	/*
	 * We allow opening, but we don't do per-open state ourselves.
	 */
	*out_openfile = NULL;
	return 0;
}

static OSKIT_COMDECL
memfsdir_open(oskit_dir_t *d, oskit_oflags_t flags,
	     struct oskit_openfile **out_opendir)
{
	if ((flags & OSKIT_O_ACCMODE) != OSKIT_O_RDONLY)
		return OSKIT_EISDIR;

	/*
	 * We allow opening, but we don't do per-open state ourselves.
	 */
	*out_opendir = NULL;
	return 0;
}


static oskit_file_t *
memfs_lookup(struct memfs *dir, const char *name)
{
	struct memfs *b;
	oskit_size_t len = strlen(name);

	if (len == 0 || (len == 1 && name[0] == '.'))
		return &dir->filei;
	if (len == 2 && name[0] == '.' && name[1] == '.')
		return (oskit_file_t *)dir->data.dir.parent ?: &dir->filei;

	for (b = dir->data.dir.contents; b; b = b->next)
		if (b->namelen == len && memcmp(name, b->name, len) == 0)
			return &b->filei;

	return NULL;
}

static OSKIT_COMDECL
memfsdir_lookup(oskit_dir_t *d, const char *name, oskit_file_t **out_file)
{
	struct memfs *dir = (void *)d;
	oskit_file_t *file;

#if VERBOSITY > 1
	osenv_local_log(dir->fsys, OSENV_LOG_INFO,
		  __FUNCTION__": asked to look up `%s' in %p\n", name, d);
#endif

	file = memfs_lookup(dir, name);
#if VERBOSITY > 2
	osenv_local_log(dir->fsys, OSENV_LOG_INFO,
		  __FUNCTION__": lookup returns %p\n", file);
#endif
	if (file) {
		oskit_file_addref(file);
		*out_file = file;
		return 0;
	}

	return OSKIT_ENOENT;
}

/*
 * Link B into DIR's contents chain.
 */
static inline void
dir_contents_link(struct memfs *dir, struct memfs *b)
{
	b->next = dir->data.dir.contents;
	b->prevp = &dir->data.dir.contents;
	if (dir->data.dir.contents)
		dir->data.dir.contents->prevp = &b->next;
	dir->data.dir.contents = b;
}

static OSKIT_COMDECL
memfsdir_create(oskit_dir_t *d, const char *name,
		oskit_bool_t excl, oskit_mode_t mode,
		oskit_file_t **out_file)
{
	struct memfs *dir = (void *)d;
	oskit_file_t *file;
	oskit_error_t rc;

#if VERBOSITY > 1
	osenv_local_log(dir->fsys, OSENV_LOG_INFO,
		  __FUNCTION__": asked to create `%s' in %p\n", name, d);
#endif

	file = memfs_lookup(dir, name);
	if (!file) {
		struct memfs *b = memfs_file_allocate(dir->fsys, 
						      name, &memfs_file_ops, 0);
		if (!b)
			rc = OSKIT_ENOSPC;
		else {
			dir_contents_link(dir, b);

			/*
			 * The directory holds one ref for the file object,
			 * and we add another for the pointer going out.
			 */
			oskit_file_addref(&b->filei);
			*out_file = &b->filei;
			rc = 0;
		}
	} else if (excl)
		rc = OSKIT_EEXIST;
	else {
		oskit_file_addref(file);
		*out_file = file;
		rc = 0;
	}

#if VERBOSITY > 2
	osenv_local_log(dir->fsys, OSENV_LOG_INFO,
		  __FUNCTION__": create returns %p (err%x)\n", *out_file, rc);
#endif
	return rc;
}

static OSKIT_COMDECL
memfsdir_link(oskit_dir_t *d, char *name, oskit_file_t *file)
{
	return OSKIT_E_NOTIMPL;
}

/*
 * Remove B from its containing directory's contents chain.
 */
static inline void
dir_contents_unlink(struct memfs *b)
{
	if (b->next)
		b->next->prevp = b->prevp;
	*b->prevp = b->next;
}

static OSKIT_COMDECL
memfsdir_unlink(oskit_dir_t *d, const char *name)
{
	struct memfs *dir = (void *)d;
	struct memfs *b;
	oskit_error_t rc;

	b = (struct memfs *)memfs_lookup(dir, name);
	if (b && b->filei.ops != (void *)&memfs_dir_ops) {
		dir_contents_unlink(b);
	}

	if (!b)
		return OSKIT_ENOENT;

	if (b->filei.ops == (void *)&memfs_dir_ops)
		rc = OSKIT_EISDIR;
	else
		rc = 0;

	memfs_file_release_internal(b);
	return rc;
}

static OSKIT_COMDECL
memfsdir_rename(oskit_dir_t *old_dir, char *old_name,
	       oskit_dir_t *new_dir, char *new_name)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
memfsdir_mkdir(oskit_dir_t *d, const char *name, oskit_mode_t mode)
{
	struct memfs *dir = (void *)d;
	oskit_file_t *file;
	oskit_error_t rc;

#if VERBOSITY > 1
	osenv_local_log(dir->fsys, OSENV_LOG_INFO,
		  __FUNCTION__": asked to mkdir `%s' in %p\n", name, d);
#endif

	file = memfs_lookup(dir, name);
	if (!file) {
		struct memfs *b = memfs_file_allocate(dir->fsys,
						      name, &memfs_dir_ops, d);
		if (!b)
			rc = OSKIT_ENOSPC;
		else {
			dir_contents_link(dir, b);
			rc = 0;
#if VERBOSITY > 2
			osenv_local_log(dir->fsys, OSENV_LOG_INFO,
				  __FUNCTION__": mkdir created %p\n", b);
#endif
		}
	} else
		rc = OSKIT_EEXIST;

#if VERBOSITY > 2
	osenv_local_log(dir->fsys, OSENV_LOG_INFO, __FUNCTION__": mkdir returns %x\n", rc);
#endif
	return rc;
}

static OSKIT_COMDECL
memfsdir_rmdir(oskit_dir_t *d, const char *name)
{
	struct memfs *dir = (void *)d;
	struct memfs *b;
	oskit_error_t rc;

#if VERBOSITY > 1
	osenv_local_log(dir->fsys, OSENV_LOG_INFO,
		  __FUNCTION__": asked to rmdir `%s' in %p\n", name, d);
#endif

	b = (struct memfs *)memfs_lookup(dir, name);

	if (!b)
		rc = OSKIT_ENOENT;
	else if ((void *)b->filei.ops != &memfs_dir_ops)
		rc = OSKIT_ENOTDIR;
	else if (b->data.dir.contents)
		rc = OSKIT_ENOTEMPTY;
	else {
		dir_contents_unlink(b);
		rc = 0;
	}

	if (rc == 0) {
		memfs_file_release_internal(b);
	}
	return rc;
}


/* oskit/fs/dir.h says:
 *
 * Read one or more entries in this directory.
 * The offset of the entry to be read is specified in 'inout_ofs';
 * the caller must initially supply zero as the offset,
 * and during each successful call the offset increases
 * by an unspecified amount dependent on the file system.
 * The file system will return at least 'nentries'
 * if there are at least this many remaining in the directory;
 * however, the file system may return more entries.
 *
 * The caller must free the dirents array and the names
 * using the task allocator when they are no longer needed.
 */
/*
 * Since nentries doesn't mean much, we'll just return everything
 * we have.
 */
static OSKIT_COMDECL
memfsdir_getdirentries(oskit_dir_t *d, oskit_u32_t *inout_ofs,
		      oskit_u32_t nentries,
		      struct oskit_dirents **out_dirents)
{
	struct memfs *dir = (void *)d;
	struct memfs *b;
	struct internal_dirent *dirents;
	int count = 0;
	oskit_error_t rc;

	/*
	 * if inout_ofs is non-zero, we assume the caller wants more entries
	 * but we've given all we have during the first call
	 */
	if (*inout_ofs != 0) {
		*out_dirents = NULL;
		return 0;
	}

	/* count files */
	for (b = dir->data.dir.contents; b; b = b->next)
		count++;


	dirents = osenv_mem_alloc(dir->fsys, sizeof dirents[0] * count,
				  OSENV_AUTO_SIZE | OSENV_NONBLOCKING,0);
	if (!dirents) {
		*out_dirents = NULL;
		return OSKIT_ENOMEM;
	}

	/*
	 * Create the dirents COM object first to make sure we can.
	 */
	if ((rc = fs_dirents_create(dir->fsys, dirents, count, out_dirents))) {
		osenv_mem_free(dir->fsys, dirents,
			       OSENV_AUTO_SIZE | OSENV_NONBLOCKING, 0);
		*out_dirents = NULL;
		return rc;
	}

	for (b = dir->data.dir.contents; b; dirents++, b = b->next) {
		oskit_size_t len = strlen(b->name) + 1;
#if VERBOSITY > 4
		osenv_local_log(dir->fsys, OSENV_LOG_INFO, __FUNCTION__": content memfs `%s'\n", b->name);
#endif
		/* copy filename in */
		dirents->name = osenv_mem_alloc(dir->fsys, len, OSENV_AUTO_SIZE | OSENV_NONBLOCKING, 0);
		if (dirents->name)
			memcpy(dirents->name, b->name, len);
#if VERBOSITY > 4
		osenv_local_log(dir->fsys, OSENV_LOG_INFO, __FUNCTION__": copy out `%s'\n", dirents->name);
#endif

		dirents->ino = (oskit_ino_t) b;
		dirents->namelen = len - 1;
	}
	*inout_ofs = 1;
	return 0;
}

static OSKIT_COMDECL
memfsdir_mknod(oskit_dir_t *d, char *name,
		oskit_mode_t mode, oskit_dev_t dev)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
memfsdir_symlink(oskit_dir_t *d, char *link_name, char *dest_name)
{
	struct memfs *dir = (void *)d;
	oskit_file_t *file;
	oskit_error_t rc;

#if VERBOSITY > 1
	osenv_local_log(dir->fsys, OSENV_LOG_INFO,
		  __FUNCTION__": asked to create symlink `%s' -> `%s' in %p\n",
		  link_name, dest_name, d);
#endif

	file = memfs_lookup(dir, link_name);
	if (file)
		rc = OSKIT_EEXIST;
	else {
		struct memfs *b = memfs_file_allocate(dir->fsys,
						      link_name,
						      &memfs_symlink_ops, 0);
		if (!b)
			rc = OSKIT_ENOSPC;
		else {
			size_t len = strlen(dest_name);
			rc = memfs_resize(b, len, 0);
			if (rc)
				memfs_file_release_internal(b);
			else {
				memcpy(b->data.file.data, dest_name, len);
				dir_contents_link(dir, b);
#if VERBOSITY > 2
				osenv_local_log(dir->fsys, OSENV_LOG_INFO,
					  __FUNCTION__": symlink created %p\n", b);
#endif
			}
		}
	}

#if VERBOSITY > 2
	osenv_local_log(dir->fsys, OSENV_LOG_INFO, __FUNCTION__": symlink returns %x\n", rc);
#endif
	return rc;
}

/*
 * Create a new "virtual" directory object
 * referring to the same underlying directory as this one,
 * but whose logical parent directory
 * (the directory named by the '..' entry in this directory)
 * is the specified directory node handle.
 *
 * If 'parent' is NULL, lookups of the '..' entry in the
 * virtual directory object will return a reference to
 * the virtual directory object itself; as with physical
 * root directories, virtual root directories will have
 * self-referential '..' entries.
 */
static OSKIT_COMDECL
memfsdir_reparent(oskit_dir_t *d, oskit_dir_t *parent, oskit_dir_t **out_dir)
{
	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
memfsfile_getfs(oskit_file_t *f, struct oskit_filesystem **out_fs)
{
	struct memfs *b = (void *)f;
	struct memfs_fsys *fsys = b->fsys;

	oskit_filesystem_addref(&fsys->fsysi);
	*out_fs = &fsys->fsysi;
	return 0;
}


static struct oskit_file_ops memfs_file_ops = {
	memfs_file_query,
	memfs_file_addref,
	memfs_file_release,
	memfsfile_stat,
	memfsfile_setstat,
	no_pathconf,
	memfsfile_sync,
	memfsfile_datasync,
	memfsfile_access,
	memfsfile_readlink,
	memfsfile_open,
	memfsfile_getfs
};

static struct oskit_dir_ops memfs_dir_ops = {
	memfs_dir_query,
	memfs_file_addref,
	memfs_file_release,
	memfsdir_stat,
	no_setstat,
	no_pathconf,
	memfsfile_sync,
	memfsfile_datasync,
	memfsfile_access,
	memfsfile_readlink,
	memfsdir_open,
	memfsfile_getfs,
	memfsdir_lookup,
	memfsdir_create,
	memfsdir_link,
	memfsdir_unlink,
	memfsdir_rename,
	memfsdir_mkdir,
	memfsdir_rmdir,
	memfsdir_getdirentries,
	memfsdir_mknod,
	memfsdir_symlink,
	memfsdir_reparent
};

/*
 * Symbolic link nodes.
 */

static OSKIT_COMDECL
memfssym_readlink(oskit_file_t *f, void *buf, oskit_u32_t len,
		 oskit_u32_t *out_actual)
{
	struct memfs *b = (void *)f;

#if VERBOSITY > 1
	osenv_local_log(b->fsys, OSENV_LOG_INFO,
		  __FUNCTION__": asked to readlink from %p\n", f);
#endif
	if (len > b->data.file.size)
		len = b->data.file.size;
	memcpy(buf, b->data.file.data, len);

	*out_actual = len;
#if VERBOSITY > 2
	osenv_local_log(b->fsys, OSENV_LOG_INFO,
		  __FUNCTION__": readlink returns `%s'\n", b->data.file.data);
#endif
	return 0;
}

static OSKIT_COMDECL
memfssym_stat(oskit_file_t *f, struct oskit_stat *st)
{
	struct memfs *b = (void *)f;

	memset(st, 0, sizeof *st);
	st->ino = (oskit_ino_t) b;
	st->size = b->data.file.size;
        st->mode = OSKIT_S_IFLNK | OSKIT_S_IRWXG | OSKIT_S_IRWXU | OSKIT_S_IRWXO;
	return 0;
}

static struct oskit_file_ops memfs_symlink_ops = {
	memfs_file_query,
	memfs_file_addref,
	memfs_file_release,
	memfssym_stat,
	no_setstat,
	no_pathconf,
	memfsfile_sync,
	memfsfile_datasync,
	memfsfile_access,
	memfssym_readlink,
	memfsfile_open,
	memfsfile_getfs
};


/*** File System interface operations ***/


/*** Operations inherited from oskit_unknown interface ***/


static OSKIT_COMDECL
memfsfilesystem_query(oskit_filesystem_t *b0,
		     const oskit_iid_t *iid, void **out_ihandle)
{
	struct memfs_fsys *b = (void *)b0;
	osenv_assert(b->count_external);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_filesystem_iid, sizeof(*iid)) == 0) {
                *out_ihandle = b;
                ++b->count_external;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
memfsfilesystem_addref(oskit_filesystem_t *b0)
{
        struct memfs_fsys *f = (void *)b0;
	osenv_assert(f->count_external);
	return ++f->count_external;
}

/* 
 * Free's a whole directory tree ignoring reference counts.
 */
static void
memfs_tree_free(struct memfs_fsys *fsys, struct memfs *b)
{
#if VERBOSITY > 20
	osenv_local_log(fsys, OSENV_LOG_INFO, __FUNCTION__": freeing '%s'\n",
			b->name);
#endif
	osenv_mem_free(fsys, b->name, OSENV_NONBLOCKING, b->namelen + 1);

	if ((void *)b->filei.ops == &memfs_dir_ops) {
		struct memfs *contents = b->data.dir.contents;
		while (contents) {
			struct memfs *x = contents;
			contents = contents->next;
			memfs_tree_free(fsys,x);
		}
	} else {
		if (b->data.file.can_sfree) {
			osenv_mem_free(fsys, b->data.file.data, OSENV_NONBLOCKING,
				       b->data.file.allocsize);
		}
	}
	osenv_mem_free(fsys, b, OSENV_NONBLOCKING, sizeof *b);
}

static OSKIT_COMDECL_U
memfsfilesystem_release(oskit_filesystem_t *b0)
{
	struct memfs_fsys *b = (void *)b0;
#ifndef KNIT
	oskit_osenv_mem_t *mem;
	oskit_osenv_log_t *log;
#endif
	osenv_assert(b->count_external);

	if (--b->count_external)
		return b->count_external;

	/* 
	 * That's it!  No more pointers exist to anywhere in the
	 * filesystem so we walk round the directory tree deleting all
	 * the file objects.
	 */

#if VERBOSITY > 20
	osenv_local_log(b, OSENV_LOG_INFO, __FUNCTION__": freeing memfs\n");
#endif
	memfs_tree_free(b,(struct memfs *)b->root);
#ifndef KNIT
	mem = b->mem;
	log = b->log;
	oskit_osenv_mem_free(mem, b, OSENV_NONBLOCKING, sizeof *b);
	oskit_osenv_mem_release(mem);
#if 0
	/* 
	 * XXX: Sigh - it might be the global object so I can't release it.
	 */
	oskit_osenv_log_release(log);
#endif
#endif
	return 0;
}


/*
 * Return general information about the file system
 * on which this node resides.
 */
static OSKIT_COMDECL
memfsfilesystem_statfs(oskit_filesystem_t *f, oskit_statfs_t *out_stats)
{
	/* XXX implement this */
        out_stats->bsize = 0;       /* file system block size */
        out_stats->frsize = 0;      /* fundamental file system block size */
        out_stats->blocks = 0;      /* total blocks in fs in units of frsize */
        out_stats->bfree = 0;       /* free blocks in fs */
        out_stats->bavail = 0;      /* free blocks avail to non-superuser */
        out_stats->files = 0;       /* total file nodes in file system */
        out_stats->ffree = 0;       /* free file nodes in fs */
        out_stats->favail = 0;      /* free file nodes avail to non-superuser */
        out_stats->fsid = 0;        /* file system id */
        out_stats->flag = 0;        /* mount flags */
        out_stats->namemax = 255;     /* max bytes in a file name */
	return 0;
}

/*
 * Write all data back to permanent storage.
 * If wait is true, this call should not return
 * until all pending data have been completely written;
 * if wait is false, begin the I/O but do not wait
 * to verify that it completes successfully.
 */
static OSKIT_COMDECL
memfsfilesystem_sync(oskit_filesystem_t *f, oskit_bool_t wait)
{
	/* oh yes, sync us */
	return 0;
}

/*
 * Return a reference to the root directory of this file system.
 */
static OSKIT_COMDECL
memfsfilesystem_getroot(oskit_filesystem_t *f, oskit_dir_t **out_dir)
{
	struct memfs_fsys *b = (void *)f;

	memfs_file_addref((oskit_file_t *)b->root);
	*out_dir = b->root;

	return 0;
}

/*
 * Update the flags of this filesystem (eg rdonly -> rdwr).
 * May return OSKIT_E_NOTIMPL if this is not
 * a typical disk-based file system.
 */
static OSKIT_COMDECL
memfsfilesystem_remount(oskit_filesystem_t *f, oskit_u32_t flags)
{
	/* no, don't remount me for now */
	return OSKIT_E_NOTIMPL;
}

/*
 * Forcibly unmount this filesystem.
 *
 * A normal unmount occurs when all references
 * to the filesystem are released.  This
 * operation forces the filesystem to be
 * unmounted, even if references still exist,
 * and is consequently unsafe.
 *
 * Subsequent attempts to use references to the
 * filesystem or to use references to files within
 * the filesystem may yield undefined results.
 */
static OSKIT_COMDECL
memfsfilesystem_unmount(oskit_filesystem_t *f)
{
	/* sure, unmount me */
	return 0;
}

static OSKIT_COMDECL
memfsfilesystem_lookupi(oskit_filesystem_t *f, oskit_ino_t ino,
		       oskit_file_t **out_file)
{
	return OSKIT_E_NOTIMPL;
}

static struct oskit_filesystem_ops memfsfilesystem_ops = {
	memfsfilesystem_query,
	memfsfilesystem_addref,
	memfsfilesystem_release,
	memfsfilesystem_statfs,
	memfsfilesystem_sync,
	memfsfilesystem_getroot,
	memfsfilesystem_remount,
	memfsfilesystem_unmount,
	memfsfilesystem_lookupi,
};

/*
 * bufio operations on memfs files
 */

static inline struct memfs *
bufio2memfs(oskit_bufio_t *io)
{
	return (struct memfs *)((char *)io - offsetof(struct memfs,
						       data.file.bufioi));
}

/* express an bufio as a pointer to a oskit_file_t */
#define bufio2memfsf(x) (&bufio2memfs(x)->filei)

static OSKIT_COMDECL
afile_query(oskit_bufio_t *io,
	    const struct oskit_guid *iid, void **out_ihandle)
{
	if (!io || io->ops != &memfs_bufio_ops)
		return OSKIT_E_INVALIDARG;

	return memfs_file_query(bufio2memfsf(io), iid, out_ihandle);
}

static OSKIT_COMDECL_U
afile_addref(oskit_bufio_t *io)
{
	if (!io || io->ops != &memfs_bufio_ops)
		return OSKIT_E_INVALIDARG;

	return memfs_file_addref(bufio2memfsf(io));
}

static OSKIT_COMDECL_U
afile_release(oskit_bufio_t *io)
{
	if (!io || io->ops != &memfs_bufio_ops)
		return OSKIT_E_INVALIDARG;

	return memfs_file_release(bufio2memfsf(io));
}


static OSKIT_COMDECL
afile_read(oskit_bufio_t *io, void *buf,
	       oskit_off_t offset, oskit_size_t amount,
	       oskit_size_t *out_actual)
{
	oskit_error_t rc;
	struct memfs *file = bufio2memfs(io);

	DMARK(file->fsys);
	if (!io || io->ops != &memfs_bufio_ops)
		return OSKIT_E_INVALIDARG;

	DMARK(file->fsys);

	if (offset > file->data.file.size)
		rc = OSKIT_EINVAL;
	else {
		DMARK(file->fsys);
		if (file->data.file.size - offset < amount)
			amount = file->data.file.size - offset;
		DMARK(file->fsys);
#if VERBOSITY > 20
		osenv_local_log(file->fsys, OSENV_LOG_INFO, __FUNCTION__": copying %d from %p+%x to %p\n",
		       amount, (char *)file->data.file.data, (int)offset,
		       buf);
#endif
		memcpy(buf, (char *)file->data.file.data + offset, amount);
		DMARK(file->fsys);
		*out_actual = amount;
		rc = 0;
	}

	return rc;
}


static OSKIT_COMDECL
afile_write(oskit_bufio_t *io, const void *buf,
		oskit_off_t offset, oskit_size_t amount,
		oskit_size_t *out_actual)
{
	oskit_error_t rc;
	struct memfs *file;

	if (!io || io->ops != &memfs_bufio_ops)
		return OSKIT_E_INVALIDARG;

	file = bufio2memfs(io);

	if (offset < 0)
		return OSKIT_EINVAL;

	if ((offset+amount) > file->data.file.size)
		rc = memfs_resize(file, offset + amount, 0);
	else
		rc = 0;

	if (!rc) {
#if VERBOSITY > 20
		osenv_local_log(file->fsys, OSENV_LOG_INFO, __FUNCTION__": copying %d from %p to %p+%x\n",
		       amount, buf, (char *)file->data.file.data, (int)offset);
#endif
		memcpy((char *)file->data.file.data + offset, buf, amount);
		*out_actual = amount;
	}

	return rc;
}


static OSKIT_COMDECL
afile_get_size(oskit_bufio_t *io, oskit_off_t *out_size)
{
	struct memfs *file;

	if (!io || io->ops != &memfs_bufio_ops)
		return OSKIT_E_INVALIDARG;

	file = bufio2memfs(io);

	*out_size = file->data.file.size;

	return 0;
}


static OSKIT_COMDECL
afile_set_size(oskit_bufio_t *io, oskit_off_t new_size)
{
	oskit_error_t rc;
	struct memfs *file;

	if (!io || io->ops != &memfs_bufio_ops || new_size < 0)
		return OSKIT_E_INVALIDARG;

	file = bufio2memfs(io);

	rc = memfs_resize(file, new_size, 0);

	return rc;
}

static OSKIT_COMDECL
afile_map(oskit_bufio_t *io, void **out_addr,
       oskit_off_t offset, oskit_size_t count)
{
	struct memfs *file;

	if (!io || io->ops != &memfs_bufio_ops || offset < 0)
		return OSKIT_E_INVALIDARG;

	/* 
	 * XXX: Does this work if someone resizes the file?
	 */
	file = bufio2memfs(io);
	if (offset + count > file->data.file.size)
		return OSKIT_E_INVALIDARG;

	*out_addr = file->data.file.data + offset;
	return 0;
}

static OSKIT_COMDECL
afile_unmap(oskit_bufio_t *io, void *addr, 
	oskit_off_t offset, oskit_size_t count)
{
	return 0;		/* consider yourself unmapped */
}

static OSKIT_COMDECL
afile_wire(oskit_bufio_t *io, oskit_addr_t *out_phys_addr, 
	oskit_off_t offset, oskit_size_t count)
{
	return 0;		/* already wired to the floor */
}

static OSKIT_COMDECL
afile_unwire(oskit_bufio_t *io, oskit_addr_t phys_addr, 
		oskit_off_t offset, oskit_size_t count)
{
	return 0;
}

static OSKIT_COMDECL
afile_copy(oskit_bufio_t *io, oskit_off_t offset, 
	oskit_size_t count, oskit_bufio_t **out_io)
{
	return OSKIT_E_NOTIMPL;
}


static struct oskit_bufio_ops memfs_bufio_ops = {
    afile_query,
    afile_addref,
    afile_release,
    (void *)0,			/* slot reserved for getblocksize */
    afile_read,
    afile_write,
    afile_get_size,
    afile_set_size,
    afile_map,
    afile_unmap,
    afile_wire,
    afile_unwire,
    afile_copy
};



#ifdef KNIT
/*
 * initialize memfs filesystem
 */
oskit_error_t
oskit_memfs_init(oskit_filesystem_t **out_fs)
{
	struct memfs_fsys *fs;

	fs = osenv_mem_alloc(DUMMY, sizeof *fs, OSENV_NONBLOCKING, 0);
	if (!fs) {
		return OSKIT_E_FAIL;
	}
	fs->count_external = 1;
	fs->fsysi.ops = &memfsfilesystem_ops;
	fs->root = (oskit_dir_t *)
		memfs_file_allocate(fs, "memfs root directory", &memfs_dir_ops, 0);
	osenv_assert(fs->root);
#if VERBOSITY > 20
	osenv_local_log(fs, OSENV_LOG_INFO, __FUNCTION__ ": created memfs root dir %p\n", fs->root);
#endif

	*out_fs = &fs->fsysi;
	return 0;
}
#else
/*
 * initialize memfs filesystem - new "extended" entrypoint
 */
oskit_error_t
oskit_memfs_init_ex(oskit_osenv_mem_t   *mem,
		    oskit_osenv_log_t   *log,
		    oskit_filesystem_t **out_fs)
{
	struct memfs_fsys *fs;

	/* 
	 * XXX: The global log is the local log of the first memfs built;
	 * XXX:   fix this when local osenvs are used.
	 */
	if (!memfs_oskit_osenv_log) {
		memfs_oskit_osenv_log = log;
		oskit_osenv_log_addref(log);
	}

	fs = oskit_osenv_mem_alloc(mem, sizeof *fs, OSENV_NONBLOCKING, 0);
	if (!fs) {
		if (mem)
			oskit_osenv_mem_release(mem);
		if (log)
			oskit_osenv_log_release(log);
		return OSKIT_E_FAIL;
	}
	fs->count_external = 1;
	fs->fsysi.ops = &memfsfilesystem_ops;
	fs->mem = mem;
	fs->log = log;
	fs->root = (oskit_dir_t *)
		memfs_file_allocate(fs, "memfs root directory", &memfs_dir_ops, 0);
	osenv_assert(fs->root);
#if VERBOSITY > 20
	osenv_local_log(fs, OSENV_LOG_INFO, __FUNCTION__ ": created memfs root dir %p\n", fs->root);
#endif

	*out_fs = &fs->fsysi;
	return 0;
}

/*
 * initialize memfs filesystem - old entrypoint
 */
oskit_error_t
oskit_memfs_init(oskit_osenv_t *osenv, oskit_filesystem_t **out_fs)
{
	oskit_osenv_mem_t *mem;
	oskit_osenv_log_t *log;

	oskit_osenv_lookup(osenv,
			   &oskit_osenv_mem_iid,
			   (void *) &mem);
	oskit_osenv_lookup(osenv,
			   &oskit_osenv_log_iid,
			   (void *) &log);
	oskit_osenv_release(osenv);

	osenv_assert(mem);
	osenv_assert(log);
	
	return oskit_memfs_init_ex(mem,log,out_fs);
}
#endif

oskit_error_t
oskit_memfs_file_set_contents(oskit_file_t *file,
			     void *data, oskit_off_t size,
			     oskit_off_t allocsize,
			     oskit_bool_t can_sfree,
			     oskit_bool_t inhibit_resize)
{
	struct memfs *b = (void *)file;
	oskit_error_t rc;

	if (file->ops != &memfs_file_ops || size > allocsize)
		return OSKIT_EINVAL;

	rc = memfs_resize(b, 0, 1);
	if (rc)
		return rc;

	b->data.file.data = data;
	b->data.file.size = size;
	b->data.file.allocsize = allocsize;
	b->data.file.can_sfree = can_sfree;
	b->data.file.inhibit_resize = inhibit_resize;

	return 0;
}

/*
 * oskit_dirents_t COM object implementation.
 */
struct fs_dirents {
	oskit_dirents_t		direntsi;	/* COM dirents interface */
	int			count;		/* Reference count */
	internal_dirent_t       *entries;	/* Array of entries */
	int			dircount;	/* Number of entries */
	int			index;		/* Current index pointer */
	struct memfs_fsys	*fsys;		/* Stash osenv stuff */
};

/*
 * Methods.
 */
static struct oskit_dirents_ops fs_dirents_ops;

static OSKIT_COMDECL
fs_dirents_query(oskit_dirents_t *d,
		 const oskit_iid_t *iid, void **out_ihandle)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;
	osenv_assert(fsdirents->count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_dirents_iid, sizeof(*iid)) == 0) {
                *out_ihandle = fsdirents;
                ++fsdirents->count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
fs_dirents_addref(oskit_dirents_t *d)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;
	osenv_assert(fsdirents->count);
	
	return ++fsdirents->count;
}

static OSKIT_COMDECL_U
fs_dirents_release(oskit_dirents_t *d)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;
	int i;
	
	osenv_assert(fsdirents->count);

	if (--fsdirents->count)
		return fsdirents->count;

	for (i = 0; i < fsdirents->dircount; i++)
		osenv_mem_free(fsdirents->fsys, fsdirents->entries[i].name,
			       OSENV_AUTO_SIZE | OSENV_NONBLOCKING, 0);

	osenv_mem_free(fsdirents->fsys, fsdirents->entries,
		       OSENV_AUTO_SIZE | OSENV_NONBLOCKING, 0);
	osenv_mem_free(fsdirents->fsys,
		       fsdirents, OSENV_NONBLOCKING, sizeof(*fsdirents));
	return 0;
}

static OSKIT_COMDECL
fs_dirents_getcount(oskit_dirents_t *d, int *out_count)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;
	osenv_assert(fsdirents->count);

	*out_count = fsdirents->dircount;

	return 0;
}

static OSKIT_COMDECL
fs_dirents_getnext(oskit_dirents_t *d, oskit_dirent_t *dirent)
{
	struct fs_dirents	*fsdirents = (struct fs_dirents *) d;
	struct internal_dirent  *idirent;
	
	osenv_assert(fsdirents->count);

	if (fsdirents->index >= fsdirents->dircount)
		return OSKIT_EWOULDBLOCK;

	idirent = &fsdirents->entries[fsdirents->index];

	if (dirent->namelen < idirent->namelen + 1)
		return OSKIT_ENAMETOOLONG;

	dirent->ino     = idirent->ino;
	dirent->namelen = idirent->namelen;
	strcpy(dirent->name, idirent->name);
	
	fsdirents->index++;	
	return 0;
}

static OSKIT_COMDECL
fs_dirents_rewind(oskit_dirents_t *d)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;
	osenv_assert(fsdirents->count);

	fsdirents->index = 0;
	return 0;
}

static struct oskit_dirents_ops fs_dirents_ops = {
	fs_dirents_query,
	fs_dirents_addref,
	fs_dirents_release,
	fs_dirents_getcount,
	fs_dirents_getnext,
	fs_dirents_rewind,
};

/*
 * Create a oskit_dirents_t COM object from an array of oskit_dirent_t
 * entries.
 */
static oskit_error_t
fs_dirents_create(struct memfs_fsys *fsys,
		  internal_dirent_t *entries,
		  int count, oskit_dirents_t **out_dirents)
{
        struct fs_dirents *fsdirents;

	fsdirents = osenv_mem_alloc(fsys, sizeof(*fsdirents),
				    OSENV_NONBLOCKING, 0);

        if (fsdirents == NULL)
                return OSKIT_ENOMEM;

        fsdirents->count        = 1;
        fsdirents->dircount     = count;
        fsdirents->entries      = entries;
        fsdirents->index        = 0;
        fsdirents->fsys         = fsys;
	fsdirents->direntsi.ops = &fs_dirents_ops;

	*out_dirents = &fsdirents->direntsi;

	return 0;
}
