/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * Implements oskit_file_t, oskit_dir_t, and oskit_openfile_t for using the
 * underlying BSD filesystem.
 *
 * We only implement the parts that make sense.
 *
 * XXXXXX: The problem spots are stat and getdirents.
 */
#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/fs/filesystem.h>
#include <oskit/io/absio.h>
#include <oskit/io/bufio.h>
#include <oskit/time.h>
#include <oskit/queue.h>

#include "native.h"
#include "support.h"

/* XXX Cannot include oskit/c/malloc.h because it pulls in too much
   other stuff. */
void sfree(void *buf, size_t size);
void* smalloc(size_t size);

#ifndef VERBOSITY
#define VERBOSITY 	0
#endif

#define	OSKIT_ALLPERMS	(OSKIT_S_ISUID|OSKIT_S_ISGID|OSKIT_S_ISVTX|\
			 OSKIT_S_IRWXU|OSKIT_S_IRWXG|OSKIT_S_IRWXO)

/*
 * structures to describe a native file or directory. Note how similar
 * they are. As it should (better) be.
 */
struct fs_file {
	oskit_file_t	filei;		/* COM file interface */
	oskit_absio_t	absioi;		/* COM absolute I/O interface */
	oskit_u32_t	count;		/* reference count */
	int		fd;		/* Native file descriptor */
	char		*pathname;	/* Path of this file */
	char		*component;	/* Component name */
	int		complen;	/* Length of component name */
	queue_chain_t	chain;		/* siblings in contents chain */

	/*
	 * Stuff unique to files.  Everything above better be the
	 * same in both.
	 *
	 * The file stuff is used for delaying file creation.
	 * See the long comment in the middle of fs_dir_create()
	 * for more information.
	 */
	oskit_mode_t	mode;		/* Mode for creation */
	char		create;		/* Needs an actual disk file created */
};

struct fs_dir {
	oskit_dir_t	diri;		/* COM directory interface */
	oskit_absio_t	absioi;		/* COM absolute I/O interface */
	oskit_u32_t	count;		/* reference count */
	int		fd;		/* Native file descriptor */
	char		*pathname;	/* Path of this directory */
	char		*component;	/* Component name */
	int		complen;	/* Length of component name */
	queue_chain_t	chain;		/* siblings in contents chain */

	/*
	 * Stuff unique to directories. Everything above better be the
	 * same in both.
	 *
	 * The contents list is a cache of fs_dir/fs_file entries. It does
	 * not necessarily reflect whats actually in the filesystem.
	 */
	oskit_dir_t	*parent;	/* logical parent; null if root dir */
	queue_head_t	contents;	/* files/dirs in this directory */
};

/*
 * An open file structure.
 */
struct fs_open {
	oskit_openfile_t ofilei;	/* COM open file interface */
	oskit_absio_t	 absioi;	/* COM absolute I/O interface */
	oskit_u32_t	 count;		/* reference count */
	struct fs_file	 *fsf;		/* Backpointer to fs_file */
	int		 fd;		/* Native file descriptor */
};

#define MAXPATHLEN		1024

static struct oskit_file_ops		fs_file_ops;
static struct oskit_dir_ops		fs_dir_ops;
static struct oskit_openfile_ops	fs_openfile_ops;
static struct fs_dir			*fs_dir_rootdir;
static oskit_error_t		fs_dirents_create(oskit_dirent_t *entries,
					int count,
					oskit_dirents_t **out_dirents);

/*
 * Create dir/file impls.
 */
static struct fs_file *
fs_file_allocate(char *pathname, const char *component)
{
        struct fs_file	*newfsf = malloc(sizeof(*newfsf));
	int		pathlen, complen;

        if (newfsf == NULL)
                return 0;

        memset(newfsf, 0, sizeof(*newfsf));

	complen = strlen(component);
	pathlen = strlen(pathname);

	if ((newfsf->pathname = malloc(complen + pathlen + 1)) == NULL) {
		free(newfsf);
		return 0;
	}
	strcpy(newfsf->pathname, pathname);
	strcat(newfsf->pathname, component);

	newfsf->component  = &newfsf->pathname[pathlen];
	newfsf->complen    = complen;
        newfsf->count      = 1;
        newfsf->filei.ops  = &fs_file_ops;
	newfsf->fd         = -1;

	return newfsf;
}

static struct fs_dir *
fs_dir_allocate(char *pathname, const char *component, oskit_dir_t *parent)
{
        struct fs_dir	*newfsd = malloc(sizeof(*newfsd));
	int		pathlen, complen;

        if (newfsd == NULL)
                return 0;

        memset(newfsd, 0, sizeof(*newfsd));

	complen = strlen(component);
	pathlen = strlen(pathname);

	if ((newfsd->pathname = malloc(complen + pathlen + 2)) == NULL) {
		free(newfsd);
		return 0;
	}
	strcpy(newfsd->pathname, pathname);
	strcat(newfsd->pathname, component);
	strcat(newfsd->pathname, "/");

	newfsd->component  = &newfsd->pathname[pathlen];
	newfsd->complen    = complen;
        newfsd->count      = 1;
        newfsd->diri.ops   = &fs_dir_ops;
	newfsd->fd         = -1;
	queue_init(&newfsd->contents);

	if (parent) {
		newfsd->parent = parent;
		oskit_dir_addref(parent);
	}
	
	return newfsd;
}

static struct fs_open *
fs_openfile_allocate(struct fs_file *fsf, int fd)
{
        struct fs_open	*ofile = malloc(sizeof(*ofile));

        if (ofile == NULL)
                return 0;

        memset(ofile, 0, sizeof(*ofile));

        ofile->count      = 1;
        ofile->ofilei.ops = &fs_openfile_ops;
	ofile->fd         = fd;
	ofile->fsf        = fsf;
	oskit_file_addref(&fsf->filei);

	return ofile;
}

/*
 * Methods.
 */
static OSKIT_COMDECL
fs_dir_query(oskit_dir_t *d, const oskit_iid_t *iid, void **out_ihandle)
{
	struct fs_dir *fsd = (struct fs_dir *) d;
	assert(fsd->count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_posixio_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_file_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_dir_iid, sizeof(*iid)) == 0) {
                *out_ihandle = fsd;
                ++fsd->count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL
fs_file_query(oskit_file_t *f, const oskit_iid_t *iid, void **out_ihandle)
{
        struct fs_file *fsf = (struct fs_file *) f;
	assert(fsf->count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_posixio_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_file_iid, sizeof(*iid)) == 0) {
                *out_ihandle = fsf;
                ++fsf->count;
                return 0;
        }

	if (memcmp(iid, &oskit_absio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &fsf->absioi;
		++fsf->count;
		return 0;
	}

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
fs_file_addref(oskit_file_t *f)
{
        struct fs_file *fsf = (struct fs_file *) f;

	assert(fsf->count);
	return ++fsf->count;
}

static OSKIT_COMDECL_U
fs_file_release(oskit_file_t *f)
{
        struct fs_file *fsf = (struct fs_file *) f;

	assert(fsf->count);

	if (--fsf->count)
		return fsf->count;

	if (fsf->fd >= 0)
		NATIVEOS(close)(fsf->fd);

	/*
	 * If the create flag is set, this was a delayed directory entry
	 * creation. Go ahead and create the file on disk now, with the
	 * mode that was specified when fs_dir_create() was called.
	 */
	if (fsf->create) {
		int fd, flags = O_WRONLY|O_CREAT|O_EXCL;

		if ((fd = NATIVEOS(open)(fsf->pathname, flags, fsf->mode)) < 0)
			panic("fs_file_release: Error opening %s: %s!",
			      fsf->pathname, strerror(native_to_oskit_error(NATIVEOS(errno))));

		NATIVEOS(close)(fd);
	}

	free(fsf->pathname);
	free(fsf);
	return 0;
}

static OSKIT_COMDECL_U
fs_dir_release(oskit_dir_t *d)
{
        struct fs_dir *fsd = (struct fs_dir *) d;

	assert(fsd->count);

	if (--fsd->count)
		return fsd->count;

	if (fsd->parent)
		oskit_dir_release(fsd->parent);
	free(fsd->pathname);
	if (fsd->fd >= 0)
		NATIVEOS(close)(fsd->fd);

	free(fsd);
	return 0;
}

/*** Operations inherited from oskit_posixio_t ***/

static OSKIT_COMDECL
fs_file_stat(oskit_file_t *f, struct oskit_stat *out_stats)
{
        struct fs_file *fsf = (struct fs_file *) f;
	struct stat sb;

#if VERBOSITY > 1
	printf(__FUNCTION__": asked to stat `%s'\n", fsf->pathname);
#endif

	if (NATIVEOS(lstat)(fsf->pathname, &sb) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

#if VERBOSITY > 1
	printf(__FUNCTION__": stat completed `%s'\n", fsf->pathname);
#endif
	out_stats->dev = sb.st_dev;
	out_stats->ino = sb.st_ino;
	out_stats->mode = sb.st_mode;
	out_stats->nlink = sb.st_nlink;
	out_stats->uid = sb.st_uid;
	out_stats->gid = sb.st_gid;
	out_stats->rdev = sb.st_rdev;
	out_stats->atime.tv_sec = STATATIMESEC(sb);
	out_stats->atime.tv_nsec = STATATIMENSEC(sb);
	out_stats->mtime.tv_sec = STATMTIMESEC(sb);
	out_stats->mtime.tv_nsec = STATMTIMENSEC(sb);
	out_stats->ctime.tv_sec = STATCTIMESEC(sb);
	out_stats->ctime.tv_nsec = STATCTIMENSEC(sb);
	out_stats->size = sb.st_size;
	out_stats->blocks = sb.st_blocks;
	out_stats->blksize = sb.st_blksize;

	return 0;
}

static OSKIT_COMDECL
fs_file_setstat(oskit_file_t *f,
		oskit_u32_t mask, const struct oskit_stat *stats)
{
        struct fs_file  *fsf = (struct fs_file *) f;
	struct stat sb;

#if VERBOSITY > 1
	printf(__FUNCTION__": asked to setstat `%s'\n", fsf->pathname);
#endif

	if (NATIVEOS(lstat)(fsf->pathname, &sb) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	/*
	 * These operations cannot be applied to a symbolic link
	 */
	if ((sb.st_mode & OSKIT_S_IFLNK) == OSKIT_S_IFLNK)
		return OSKIT_E_NOTIMPL;

	/*
	 * Okay, dispatch on how he caller wants to change the stat structure.
	 * Only allow a few things right now.
	 */
	if (mask & ~(OSKIT_STAT_MODE|OSKIT_STAT_UID|
		     OSKIT_STAT_SIZE|OSKIT_STAT_GID|
		     OSKIT_STAT_ATIME|OSKIT_STAT_MTIME))
		return OSKIT_E_INVALIDARG;

	if (mask & OSKIT_STAT_MODE) {
		if (NATIVEOS(chmod)(fsf->pathname, /* 07777 are portable */
				    stats->mode & OSKIT_ALLPERMS) < 0)
			return native_to_oskit_error(NATIVEOS(errno));
	}
	if (mask & OSKIT_STAT_UID) {
		if (NATIVEOS(chown)(fsf->pathname, stats->uid, -1) < 0)
			return native_to_oskit_error(NATIVEOS(errno));
	}
	if (mask & OSKIT_STAT_GID) {
		if (NATIVEOS(chown)(fsf->pathname, -1, stats->gid) < 0)
			return native_to_oskit_error(NATIVEOS(errno));
	}
	if (mask & OSKIT_STAT_SIZE) {
		if (NATIVEOS(truncate)(fsf->pathname, stats->size) < 0)
			return native_to_oskit_error(NATIVEOS(errno));
	}
	if (mask & (OSKIT_STAT_ATIME|OSKIT_STAT_MTIME)) {
		struct timeval timevals[2];

		if (mask & OSKIT_STAT_ATIME) {
			TIMESPEC_TO_TIMEVAL(&timevals[0], &stats->atime);
		}
		else {
			NATIVEOS(gettimeofday)(&timevals[0], NULL);
		}

		if (mask & OSKIT_STAT_MTIME) {
			TIMESPEC_TO_TIMEVAL(&timevals[1], &stats->mtime);
		}
		else {
			NATIVEOS(gettimeofday)(&timevals[1], NULL);
		}

		if (NATIVEOS(utimes)(fsf->pathname, timevals) < 0)
			return native_to_oskit_error(NATIVEOS(errno));
	}

	return 0;
}

static OSKIT_COMDECL
fs_file_pathconf(oskit_file_t *f, oskit_s32_t option, oskit_s32_t *out_val)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_file_sync(oskit_file_t *f, oskit_bool_t wait)
{
        struct fs_file  *fsf = (struct fs_file *) f;

	assert(fsf->fd);

	if (NATIVEOS(fsync)(fsf->fd) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	return 0;
}

static OSKIT_COMDECL
fs_file_access(oskit_file_t *f, oskit_amode_t mask)
{
        struct fs_file  *fsf = (struct fs_file *) f;

#if VERBOSITY > 1
	printf(__FUNCTION__": asked to access on `%s'\n", fsf->pathname);
#endif
	if (NATIVEOS(access)(fsf->pathname, mask) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	return 0;
}

static OSKIT_COMDECL
fs_file_readlink(oskit_file_t *f, char *buf, oskit_u32_t len,
		 oskit_u32_t *out_actual)
{
        struct fs_file  *fsf = (struct fs_file *) f;
	int		count;

	if ((count = NATIVEOS(readlink)(fsf->pathname, buf, len)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	*out_actual = count;

	return 0;
}

static OSKIT_COMDECL
fs_file_open(oskit_file_t *f, oskit_oflags_t iflags,
	     struct oskit_openfile **out_openfile)
{
        struct fs_file  *fsf = (struct fs_file *) f;
	struct fs_open  *fso;
	int		mode = 0, flags = 0, fd;

#if VERBOSITY > 1
	printf(__FUNCTION__": asked to open `%s'\n", fsf->pathname);
#endif
	flags = 0;
	if ((iflags & OSKIT_O_ACCMODE) == OSKIT_O_RDONLY)
		flags = O_RDONLY;
	if ((iflags & OSKIT_O_ACCMODE) == OSKIT_O_WRONLY)
		flags = O_WRONLY;
	if ((iflags & OSKIT_O_ACCMODE) == OSKIT_O_RDWR)
		flags = O_RDWR;

	if (iflags & OSKIT_O_TRUNC)
		flags |= O_TRUNC;
	if (iflags & OSKIT_O_APPEND)
		flags |= O_APPEND;
	if (iflags & OSKIT_O_CREAT)
		flags |= O_CREAT;

	/*
	 * If the create flag is set, this was a delayed directory entry
	 * creation. Use the mode bits that were specified when
	 * fs_dir_create() was called so that file is created with the
	 * proper bits. Add in O_EXCL too just to be safe.
	 */
	if (fsf->create) {
		flags |= O_CREAT|O_EXCL;
		mode   = fsf->mode;
		fsf->create = 0;
	}

	if ((fd = NATIVEOS(open)(fsf->pathname, flags, mode)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	/*
	 * Open succeeded, so create an openfile for it.
	 */
	if ((fso = fs_openfile_allocate(fsf, fd)) == NULL) {
		NATIVEOS(close)(fd);
		return OSKIT_ENOMEM;
	}

	*out_openfile = &fso->ofilei;
	return 0;
}

static OSKIT_COMDECL
fs_dir_open(oskit_dir_t *d, oskit_oflags_t iflags,
	    struct oskit_openfile **out_opendir)
{
        struct fs_dir  *fsd = (struct fs_dir *) d;
	struct fs_open *fso;
	int		flags, fd;

	flags = 0;
	if ((iflags & OSKIT_O_ACCMODE) == OSKIT_O_RDONLY)
		flags = O_RDONLY;
	if ((iflags & OSKIT_O_ACCMODE) == OSKIT_O_WRONLY)
		flags = O_WRONLY;
	if ((iflags & OSKIT_O_ACCMODE) == OSKIT_O_RDWR)
		flags = O_RDWR;

	if (iflags & OSKIT_O_TRUNC)
		flags |= O_TRUNC;
	if (iflags & OSKIT_O_APPEND)
		flags |= O_APPEND;
	if (iflags & OSKIT_O_CREAT)
		flags |= O_CREAT;

	if ((fd = NATIVEOS(open)(fsd->pathname, flags, 0)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	/*
	 * Open succeeded, so create an openfile for it.
	 */
	if ((fso = fs_openfile_allocate((struct fs_file *)fsd, fd)) == NULL) {
		NATIVEOS(close)(fd);
		return OSKIT_ENOMEM;
	}

	*out_opendir = &fso->ofilei;
	return 0;
}

/*
 * Lookup a file/dir in the contents list for a directory.
 */
static oskit_file_t *
fs_lookup(struct fs_dir *fsd, const char *name)
{
	oskit_size_t	len  = strlen(name);
        struct fs_file	*fsf;

	if (len == 0 || (len == 1 && name[0] == '.'))
		return (oskit_file_t *) &fsd->diri;

	if (len == 2 && name[0] == '.' && name[1] == '.')
		return (oskit_file_t *) (fsd->parent ?: &fsd->diri);

	queue_iterate(&fsd->contents, fsf, struct fs_file *, chain) {
		if (fsf->complen == len &&
		    memcmp(name, fsf->component, len) == 0)
			return &fsf->filei;
	}

	return NULL;
}

/*
 * Link F into DIR's contents chain.
 */
static inline void
fs_contents_link(struct fs_dir *fsd, struct fs_dir *f)
{
	queue_enter_first(&fsd->contents, f, struct fs_dir *, chain);
}

/*
 * Remove F from its containing directory's contents chain.
 */
static inline void
fs_contents_unlink(struct fs_dir *fsd, struct fs_dir *f)
{
	queue_remove(&fsd->contents, f, struct fs_dir *, chain);
}

static OSKIT_COMDECL
fs_dir_lookup(oskit_dir_t *d, const char *name, oskit_file_t **out_file)
{
        struct fs_dir	*fsd = (struct fs_dir *) d;
	struct fs_file  *fsf;
	oskit_file_t	*file;
	struct stat	sb;
	char		fname[MAXPATHLEN];

#if VERBOSITY > 1
	printf(__FUNCTION__": asked to look up `%s' in `%s'\n",
	       name, fsd->pathname);
#endif

	file = fs_lookup(fsd, name);
	if (file) {
#if VERBOSITY > 1
		struct fs_file	*fsf = (struct fs_file *) file;

		printf(__FUNCTION__": Found it `%s'\n", fsf->pathname);
#endif
		oskit_file_addref(file);
		*out_file = file;
		return 0;
	}

	/*
	 * Okay, now look on the disk to see if exists. Use stat since we
	 * need to know if the component is a file or directory.
	 */
	strcpy(fname, fsd->pathname);
	strcat(fname, name);
	if (NATIVEOS(lstat)(fname, &sb) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

#if VERBOSITY > 1
	printf(__FUNCTION__": On disk: `%s'\n", fname);
#endif

	/*
	 * It does. Create a fs_file or fs_dir entry and link it in.
	 */
	if (sb.st_mode & OSKIT_S_IFDIR)
		fsf = (struct fs_file *)
			fs_dir_allocate(fsd->pathname, name, d);
	else
		fsf = fs_file_allocate(fsd->pathname, name);

	if (fsf == NULL)
		return OSKIT_ENOMEM;

	fs_contents_link(fsd, (struct fs_dir *) fsf);

	/*
	 * The directory holds one ref for the file object,
	 * and we add another for the pointer going out.
	 */
	++fsf->count;
	*out_file = &fsf->filei;
	return 0;
}

static OSKIT_COMDECL
fs_dir_create(oskit_dir_t *d, const char *name,
	       oskit_bool_t excl, oskit_mode_t mode,
	       oskit_file_t **out_file)
{
        struct fs_dir	*fsd = (struct fs_dir *) d;
	oskit_file_t	*file;
	oskit_error_t	rc;

#if VERBOSITY > 1
	printf(__FUNCTION__": asked to create `%s'\n", name);
#endif
	file = fs_lookup(fsd, name);
	if (!file) {
		struct fs_file *fsf;

		fsf = fs_file_allocate(fsd->pathname, name);

		if (!fsf) {
			rc = OSKIT_ENOMEM;
			goto done;
		}

		fs_contents_link(fsd, (struct fs_dir *) fsf);

		/*
		 * See if the file already exists on disk. With excl set
		 * it must not exist.
		 */
		if (NATIVEOS(access)(fsf->pathname, 0) == 0) {
			/*
			 * It exists. We created a directory entry, but
			 * that is okay. Return error.
			 */
			if (excl) {
				rc = OSKIT_EEXIST;
				goto done;
			}
			/*
			 * Otherwise, return a handle to the directory entry.
			 *
			 * The directory holds one ref for the file object,
			 * and we add another for the pointer going out.
			 */
			++fsf->count;
			*out_file = &fsf->filei;
			rc = 0;
			goto done;
		}

		/*
		 * The file does not exist. We could create it, but that
		 * might result in a situation in which a file is created
		 * with a mode that does not allow it to be opened (easily)
		 * later. This is exactly the situation when called from
		 * libc open(). The entry is first created with a mode, and
		 * then opened with a set of flags. If the mode is rx, but
		 * the flags say writeable, we run into a minor problem.
		 * The easiest thing to do is just delay until until either
		 a file_open is called, or the file is released.
		 * Semantically incorrect, but this *is* supposed to be
		 * a simple emulation. Even worse, all of the file operation
		 * calls should check the flag too (ie: create, followed by
		 * stat), and create the file. But, the only existing use of
		 * oskit_dir_create is libc open, so lets not bother.
		 */
		fsf->create = 1;
		fsf->mode   = mode;

		/*
		 * The directory holds one ref for the file object,
		 * and we add another for the pointer going out.
		 */
		++fsf->count;
		*out_file = &fsf->filei;
		rc = 0;
	}
	else if (excl)
		rc = OSKIT_EEXIST;
	else {
		oskit_file_addref(file);
		*out_file = file;
		rc = 0;
	}
   done:
	return rc;
}

static OSKIT_COMDECL
fs_dir_link(oskit_dir_t *d, const char *name, oskit_file_t *file)
{
        struct fs_dir	*fsd = (struct fs_dir *) d;
        struct fs_file	*fsf = (struct fs_file *) file;
        struct fs_file	*newfsf;

	/*
	 * Target must not exist.
	 */
	if ((newfsf = (struct fs_file *) fs_lookup(fsd, name)) != NULL)
		return OSKIT_EEXIST;

	if ((newfsf = fs_file_allocate(fsd->pathname, name)) == NULL)
		return OSKIT_ENOMEM;

#if VERBOSITY > 1
	printf(__FUNCTION__": asked to link `%s' to `%s'\n",
	       newfsf->pathname, fsf->pathname);
#endif
	/*
	 * Now try making the actual link.
	 */
	if (NATIVEOS(link)(fsf->pathname, newfsf->pathname) < 0) {
		oskit_file_release(&newfsf->filei);
		return native_to_oskit_error(NATIVEOS(errno));
	}

	/*
	 * Success, so link it into the directory.
	 */
	fs_contents_link(fsd, (struct fs_dir *) newfsf);

	return 0;
}

static OSKIT_COMDECL
fs_dir_unlink(oskit_dir_t *d, const char *name)
{
        struct fs_dir	*fsd = (struct fs_dir *) d;
        struct fs_file	*fsf;
	char		fname[MAXPATHLEN];

	fsf = (struct fs_file *) fs_lookup(fsd, name);

	/*
	 * Not finding a file entry does not mean the file does not
	 * exist! Only that we don't have a handle cached.
	 */
	if (fsf) {
		if (fsf->filei.ops == (void *)&fs_dir_ops)
			return OSKIT_EISDIR;

		fs_contents_unlink(fsd, (struct fs_dir *) fsf);
		fs_file_release((oskit_file_t *) fsf);
	}

	/*
	 * Now do the disk unlink and let it return error if the file
	 * does not exist. This is slightly inconsistent, but okay.
	 */
	strcpy(fname, fsd->pathname);
	strcat(fname, name);
	if (NATIVEOS(unlink)(fname) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	return 0;
}

static OSKIT_COMDECL
fs_dir_rename(oskit_dir_t *old_dir, const char *old_name,
	      oskit_dir_t *new_dir, const char *new_name)
{
        struct fs_dir	*old_fsd = (struct fs_dir *) old_dir;
        struct fs_dir	*new_fsd = (struct fs_dir *) new_dir;
        struct fs_file	*new_fsf;  /* may be an fs_dir */
        struct fs_file	*old_fsf;  /* may be an fs_dir */
	char		old_fname[MAXPATHLEN];
	char		new_fname[MAXPATHLEN];
	int		old_fsf_is_dir;

	old_fsf_is_dir = -1;
	strcpy(old_fname, old_fsd->pathname);
	strcat(old_fname, old_name);

	old_fsf = (struct fs_file *) fs_lookup(old_fsd, old_name);
	if (!old_fsf) {
		struct stat sb;
		int rc;
		
		/*
		 * Not finding the file does not mean that the file
		 * does not exist!  Only that we don't have a handle cached.
		 */
		rc = NATIVEOS(lstat)(old_fname, &sb);
		if (rc)
			/* doesn't matter what actually went wrong... */
			return OSKIT_ENOENT; 

		old_fsf_is_dir = (sb.st_mode & OSKIT_S_IFDIR);
	} 
	else {
		old_fsf_is_dir = (old_fsf->filei.ops == (void*)&fs_dir_ops);
	}
	

	new_fsf = (struct fs_file *) fs_lookup(new_fsd, new_name);

	/*
	 * Release our handle on it before doing the actual disk
	 * operation.
	 */
	if (new_fsf) {
		fs_contents_unlink(new_fsd, (struct fs_dir *) new_fsf);

		/* new_fsf may be a file or a directory, so free it */
		if (new_fsf->filei.ops == (void*)&fs_dir_ops)
			fs_dir_release((oskit_dir_t *) new_fsf);
		else
			fs_file_release((oskit_file_t *) new_fsf);
	}

	/*
	 * Now do the disk rename and let it return any errors.
	 * This is slightly inconsistent, but okay. 
	 */
	strcpy(new_fname, new_fsd->pathname);
	strcat(new_fname, new_name);
	if (NATIVEOS(rename)(old_fname, new_fname) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	/*
	 * Operation success. Release the source file now that disk has
	 * really been changed, if we had a handle, that is.
	 */
	if (old_fsf) {
		fs_contents_unlink(old_fsd, (struct fs_dir *) old_fsf);

		if (old_fsf_is_dir)
			fs_dir_release((oskit_dir_t *) old_fsf);
		else
			fs_file_release((oskit_file_t *) old_fsf);
	}

	return 0;
}

static OSKIT_COMDECL
fs_dir_mkdir(oskit_dir_t *d, const char *name, oskit_mode_t mode)
{
        struct fs_dir	*fsd = (struct fs_dir *) d;
        struct fs_dir	*newfsd;
	oskit_file_t	*file;
	char		fname[MAXPATHLEN];

#if VERBOSITY > 1
	printf(__FUNCTION__": asked to mkdir `%s'\n", name);
#endif
	if ((file = fs_lookup(fsd, name)) != NULL)
		return OSKIT_EEXIST;

	/*
	 * First see if we can actually make the directory on disk.
	 */
	strcpy(fname, fsd->pathname);
	strcat(fname, name);
	if (NATIVEOS(mkdir)(fname, mode) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	/*
	 * Yep, so now make the fs_dir structure and link it in.
	 */
	if ((newfsd = fs_dir_allocate(fsd->pathname, name, d)) == NULL)
		return OSKIT_ENOMEM;

	fs_contents_link(fsd, newfsd);
	return 0;
}

static OSKIT_COMDECL
fs_dir_rmdir(oskit_dir_t *d, const char *name)
{
        struct fs_dir	*fsd = (struct fs_dir *) d;
        struct fs_dir	*remfsd;
	char		fname[MAXPATHLEN];

#if VERBOSITY > 1
	printf(__FUNCTION__": asked to rmdir `%s'\n", name);
#endif

	remfsd = (struct fs_dir *) fs_lookup(fsd, name);

	/*
	 * Not finding a dir entry does not mean the directory does not
	 * exist! Only that we don't have a handle cached.
	 */
	if (remfsd) {
		if (remfsd->diri.ops != (void *)&fs_dir_ops)
			return OSKIT_ENOTDIR;

		if (! queue_empty(&remfsd->contents))
			return OSKIT_ENOTEMPTY;

		fs_contents_unlink(fsd, remfsd);
		fs_dir_release((oskit_dir_t *) remfsd);
	}

	/*
	 * Now do the actual rmdir and let it return error.
	 */
	strcpy(fname, fsd->pathname);
	strcat(fname, name);
	if (NATIVEOS(rmdir)(fname) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	return 0;
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
fs_dir_getdirentries(oskit_dir_t *d, oskit_u32_t *inout_ofs,
		     oskit_u32_t nentries,
		     struct oskit_dirents **out_dirents)
{
        struct fs_dir		*fsd = (struct fs_dir *) d;
	char			buf[0x1000];
#ifdef __linux__
	off_t			bsize;
#else
	long			bsize;
#endif
	int			count, doff, dcnt, i;
	struct oskit_dirent	*dirents;
	oskit_size_t		dirents_size;
	struct dirent		*dent;
	oskit_error_t		rc;
	off_t			offset;

	/*
	 * This is implemented using the underlying filesystem only.
	 * We don't normally open directories, so check for that and
	 * for a directory rewind. Special flag is -1, which says that
	 * the last call returned all there was to read.
	 */
	if (fsd->fd < 0) {
		if ((fsd->fd = NATIVEOS(open)(fsd->pathname, O_RDONLY, 0)) < 0)
			return native_to_oskit_error(NATIVEOS(errno));
	}

	/*
	 * basep is not read by getdirents.  At the end of the call,
	 * it is set to the file position of the first entry.  Always
	 * reset file pos, in case the file was closed or seeked.
	 *
	 * This behavior is documented in the freebsd manpage.  The
	 * linux manpage says something else, but acts like BSD.
	 */
	NATIVEOS(lseek)(fsd->fd, *inout_ofs, SEEK_SET);

	/*
	 * Ask for a fixed size block of entries, and then convert that
	 * into Oskit dirent structures.
	 */
	bsize = *inout_ofs;
	if ((count =
	     NATIVEOS(getdirentries)(fsd->fd, buf, sizeof buf, &bsize)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	/*
	 * No entries at all, so close it down.
	 */
	if (count == 0) {
		NATIVEOS(close)(fsd->fd);
		fsd->fd = -1;
		*out_dirents = NULL;
		return 0;
	}

	offset = NATIVEOS(lseek)(fsd->fd, 0, SEEK_CUR);
	if (offset < 0)
		return native_to_oskit_error(NATIVEOS(errno));
	*inout_ofs = offset;

	/*
	 * Count them up.
	 */
	dent = (struct dirent *) buf;
	dirents_size = 0;
	doff = 0;
	dcnt = 0;
	while (doff < count) {
		/* XXX kludge that is always too big, but well-aligned */
		dirents_size += offsetof(oskit_dirent_t, name[dent->d_reclen]);
		doff += dent->d_reclen;
		dent  = (struct dirent *) &buf[doff];
		++dcnt;
	}

	dirents = malloc(dirents_size);
	if (!dirents) {
		*out_dirents = NULL;
		return OSKIT_ENOMEM;
	}

	/*
	 * Create the dirents COM object first to make sure we can.
	 */
	if ((rc = fs_dirents_create(dirents, dcnt, out_dirents))) {
		free(dirents);
		*out_dirents = NULL;
		return rc;
	}

	dent = (struct dirent *) buf;
	for (i = 0, doff = 0; i < dcnt; i++) {
		/* copy filename in */
		dirents->namelen = DIRENTNAMLEN(dent);
		dirents->ino = DIRENTINO(dent);
		memcpy(dirents->name, dent->d_name, DIRENTNAMLEN(dent) + 1);
		dirents = (void *)(&dirents->name[dent->d_reclen]);
		doff += dent->d_reclen;
		dent  = (struct dirent *) &buf[doff];
	}

	return 0;
}

static OSKIT_COMDECL
fs_dir_mknod(oskit_dir_t *d, const char *name, oskit_mode_t mode,
	     oskit_dev_t dev)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_dir_symlink(oskit_dir_t *d, const char *link_name, const char *dest_name)
{
        struct fs_dir	*fsd = (struct fs_dir *) d;
	char		fname[MAXPATHLEN];

#if VERBOSITY > 1
	printf(__FUNCTION__": asked to create symlink `%s' -> `%s'\n",
	       link_name, dest_name);
#endif

	/*
	 * Lets just create the symlink and not worry about caching
	 * a handle on it.
	 */
	strcpy(fname, fsd->pathname);
	strcat(fname, link_name);

	if (NATIVEOS(symlink(dest_name, fname)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	return 0;
}

static OSKIT_COMDECL
fs_dir_reparent(oskit_dir_t *d, oskit_dir_t *parent, oskit_dir_t **out_dir)
{
	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
fs_file_getfs(oskit_file_t *f, struct oskit_filesystem **out_fs)
{
	return OSKIT_E_NOTIMPL;	/* XXX implement  */
}

static struct oskit_file_ops fs_file_ops = {
	fs_file_query,
	fs_file_addref,
	fs_file_release,
	fs_file_stat,
	fs_file_setstat,
	fs_file_pathconf,
	fs_file_sync,
	fs_file_sync,
	fs_file_access,
	fs_file_readlink,
	fs_file_open,
	fs_file_getfs
};

static struct oskit_dir_ops fs_dir_ops = {
	fs_dir_query,
	fs_file_addref,
	fs_dir_release,
	fs_file_stat,
	fs_file_setstat,
	fs_file_pathconf,
	fs_file_sync,
	fs_file_sync,
	fs_file_access,
	fs_file_readlink,
	fs_dir_open,
	fs_file_getfs,
	fs_dir_lookup,
	fs_dir_create,
	fs_dir_link,
	fs_dir_unlink,
	fs_dir_rename,
	fs_dir_mkdir,
	fs_dir_rmdir,
	fs_dir_getdirentries,
	fs_dir_mknod,
	fs_dir_symlink,
	fs_dir_reparent
};

/*
 * oskit_openfile methods
 */
static OSKIT_COMDECL
fs_ofile_query(oskit_openfile_t *f,
	       const struct oskit_guid *iid, void **out_ihandle)
{
	struct fs_open	*ofile = (struct fs_open *) f;

	assert(ofile);
	assert(ofile->count != 0);

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_openfile_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &ofile->ofilei;
		++ofile->count;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
fs_ofile_addref(oskit_openfile_t *f)
{
	struct fs_open	*ofile = (struct fs_open *) f;

	assert(ofile);
	assert(ofile->count != 0);

	return ++ofile->count;
}


static OSKIT_COMDECL_U
fs_ofile_release(oskit_openfile_t *f)
{
	struct fs_open	*ofile = (struct fs_open *) f;
	unsigned	newcount;

	assert(ofile);
	assert(ofile->count != 0);

	newcount = --ofile->count;
	if (newcount == 0) {
		oskit_file_release(&ofile->fsf->filei);

		NATIVEOS(close)(ofile->fd);
		free(ofile);
	}

	return newcount;
}

static OSKIT_COMDECL
fs_ofile_read(oskit_openfile_t *f,
	      void *buf, oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct fs_open	*ofile = (struct fs_open *) f;
	int		count;

	assert(ofile);
	assert(ofile->count != 0);

	if ((count = NATIVEOS(read)(ofile->fd, buf, len)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	*out_actual = count;
	return 0;
}

static OSKIT_COMDECL
fs_ofile_write(oskit_openfile_t *f,
	       const void *buf, oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct fs_open	*ofile = (struct fs_open *) f;
	int		count;

	assert(ofile);
	assert(ofile->count != 0);

	if ((count = NATIVEOS(write)(ofile->fd, buf, len)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	*out_actual = count;
	return 0;
}

static OSKIT_COMDECL
fs_ofile_seek(oskit_openfile_t *f,
	      oskit_s64_t ofs, oskit_seek_t whence, oskit_u64_t *out_newpos)
{
	struct fs_open	*ofile = (struct fs_open *) f;
	oskit_u64_t	newpos;

	assert(ofile);
	assert(ofile->count != 0);

	if ((newpos = NATIVEOS(lseek)(ofile->fd, ofs, whence)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	*out_newpos = newpos;
	return 0;
}

static OSKIT_COMDECL
fs_ofile_setsize(oskit_openfile_t *f, oskit_u64_t new_size)
{
	struct fs_open	*ofile = (struct fs_open *) f;
	oskit_u64_t	oldpos;

	assert(ofile);
	assert(ofile->count != 0);

	if ((oldpos = NATIVEOS(lseek)(ofile->fd, new_size, SEEK_SET)) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	if (NATIVEOS(lseek)(ofile->fd, oldpos, SEEK_SET) < 0)
		return native_to_oskit_error(NATIVEOS(errno));

	return 0;
}


static OSKIT_COMDECL
fs_ofile_copy_to(oskit_openfile_t *f, oskit_stream_t *dst,
		 oskit_u64_t size, oskit_u64_t *out_read,
		 oskit_u64_t *out_written)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_ofile_commit(oskit_openfile_t *f, oskit_u32_t commit_flags)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_ofile_revert(oskit_openfile_t *f)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_ofile_lock_region(oskit_openfile_t *f,
		     oskit_u64_t offset, oskit_u64_t size,
		     oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_ofile_unlock_region(oskit_openfile_t *f,
		    oskit_u64_t offset, oskit_u64_t size,
		    oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_ofile_stat(oskit_openfile_t *f,
	      oskit_stream_stat_t *out_stat, oskit_u32_t stat_flags)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_ofile_clone(oskit_openfile_t *f, oskit_openfile_t **out_stream)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_ofile_getfile(oskit_openfile_t *f, struct oskit_file **out_file)
{
	struct fs_open	*ofile = (struct fs_open *) f;

	assert(ofile);
	assert(ofile->count != 0);

	oskit_file_addref(&ofile->fsf->filei);
	*out_file = &ofile->fsf->filei;

	return 0;
}

static struct oskit_openfile_ops fs_openfile_ops = {
	fs_ofile_query,
	fs_ofile_addref,
	fs_ofile_release,
	fs_ofile_read,
	fs_ofile_write,
	fs_ofile_seek,
	fs_ofile_setsize,
	fs_ofile_copy_to,
	fs_ofile_commit,
	fs_ofile_revert,
	fs_ofile_lock_region,
	fs_ofile_unlock_region,
	fs_ofile_stat,
	fs_ofile_clone,
	fs_ofile_getfile
};

/*
 * Create a directory, rooted at the given pathname.
 */
oskit_dir_t *
oskit_unix_fs_init(char *rootname)
{
        struct fs_dir	*newfsd = malloc(sizeof(*newfsd));
	int		pathlen;

	/*
	 * Make sure the path exists.
	 */
	if (NATIVEOS(access)(rootname, 0) < 0)
		panic("oskit_unix_fs_init: Cannot access root `%s'", rootname);

        if (newfsd == NULL)
		panic("oskit_unix_fs_init: Out of memory");

        memset(newfsd, 0, sizeof(*newfsd));

	pathlen = strlen(rootname);

	if ((newfsd->pathname = malloc(pathlen + 2)) == NULL) {
		free(newfsd);
		return 0;
	}
	strcpy(newfsd->pathname, rootname);
	if (newfsd->pathname[strlen(newfsd->pathname) - 1] != '/')
	    strcat(newfsd->pathname, "/");

	newfsd->component  = NULL;
	newfsd->complen    = 0;
        newfsd->count      = 1;
        newfsd->diri.ops   = &fs_dir_ops;
	newfsd->fd         = -1;
	queue_init(&newfsd->contents);

	fs_dir_rootdir = newfsd;

	return (oskit_dir_t *)newfsd;
}

/*
 * Debug. Print the entire tree.
 */
static void
fs_print_file(struct fs_file *fsf)
{
	printf("F: 0x%x fd:%d %s\n", (int) fsf, fsf->fd, fsf->pathname);
}

static void
fs_print_dir(struct fs_dir *fsd)
{
	struct fs_file *fsf;

	printf("D: 0x%x fd:%d %s\n", (int) fsd, fsd->fd, fsd->pathname);

	queue_iterate(&fsd->contents, fsf, struct fs_file *, chain) {
		if (fsf->filei.ops == (void *)&fs_dir_ops)
			fs_print_dir((struct fs_dir *) fsf);
		else
			fs_print_file(fsf);
	}
}

void
fs_dumptree()
{
	fs_print_dir(fs_dir_rootdir);
}

/*
 * oskit_dirents_t COM object implementation.
 */
struct fs_dirents {
	oskit_dirents_t		direntsi;	/* COM dirents interface */
	int			count;		/* Reference count */
	oskit_dirent_t       *entries;	/* Array of entries */
	int			dircount;	/* Number of entries */
	int			index;		/* Current index pointer */
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
	assert(fsdirents->count);

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
	assert(fsdirents->count);

	return ++fsdirents->count;
}

static OSKIT_COMDECL_U
fs_dirents_release(oskit_dirents_t *d)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;

	assert(fsdirents->count);

	if (--fsdirents->count)
		return fsdirents->count;

	/* fsdirentries->entries is one contiguous block of variable
	 * sized ents, so don't free individual bits of it.
	 */

	free(fsdirents->entries);
	sfree(fsdirents, sizeof(*fsdirents));
	return 0;
}

static OSKIT_COMDECL
fs_dirents_getcount(oskit_dirents_t *d, int *out_count)
{
	struct fs_dirents *fsdirents = (struct fs_dirents *) d;
	assert(fsdirents->count);

	*out_count = fsdirents->dircount;

	return 0;
}

static OSKIT_COMDECL
fs_dirents_getnext(oskit_dirents_t *d, oskit_dirent_t *dirent)
{
	struct fs_dirents	*fsdirents = (struct fs_dirents *) d;
	struct oskit_dirent  *idirent;

	assert(fsdirents->count);

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
	assert(fsdirents->count);

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
fs_dirents_create(oskit_dirent_t *entries,
		  int count, oskit_dirents_t **out_dirents)
{
        struct fs_dirents *fsdirents = smalloc(sizeof(*fsdirents));

        if (fsdirents == NULL)
                return OSKIT_ENOMEM;

        fsdirents->count        = 1;
        fsdirents->dircount     = count;
        fsdirents->entries      = entries;
        fsdirents->index        = 0;
	fsdirents->direntsi.ops = &fs_dirents_ops;

	*out_dirents = &fsdirents->direntsi;

	return 0;
}
