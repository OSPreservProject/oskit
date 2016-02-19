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
 * Implement a virtual directory to be mounted under /dev, and which
 * converts opens into a call into the device code. We export an
 * absio interface which can then be converted into an openfile_t using
 * an soa adaptor. What the caller gets back then, is an openfile_t that
 * talks to a raw device and can be used to read/write into like a character
 * device.
 *
 * At the moment, we only deal with disk drivers, passing the open off into
 * the Linux device drivers. At some point this could be much smarter, by
 * looking at the registered devices and trying to match any arbitrary
 * device open request to an installed device. Someday ...
 */

#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/fs/filesystem.h>
#include <oskit/io/absio.h>
#include <oskit/io/bufio.h>
#include <oskit/time.h>
#include <oskit/queue.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/bus.h>
#include <oskit/dev/tty.h>
#include <oskit/dev/blk.h>
#include <oskit/dev/ethernet.h>
#include <oskit/startup.h>
#include <oskit/dev/linux.h>
#include <oskit/diskpart/diskpart.h>
#include <oskit/c/fs.h>

#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/stat.h>

#ifndef VERBOSITY
#define VERBOSITY 	0
#endif

/*
 * structures to describe a native file or directory. Note how similar
 * they are. As it should (better) be.
 */
struct fs_file {
	oskit_file_t	filei;		/* COM file interface */
	oskit_absio_t	absioi;		/* COM absolute I/O interface */
	oskit_u32_t	count;		/* reference count */
	char		*component;	/* Component name */
	oskit_blkio_t   *bio;		/* Block I/O of open disk/part */
	oskit_u32_t	blksize;	/* For sanity check */
};

struct fs_dir {
	oskit_dir_t	diri;		/* COM directory interface */
	oskit_absio_t	absioi;		/* COM absolute I/O interface */
	oskit_u32_t	count;		/* reference count */
	char		*component;	/* Component name */
};

static struct oskit_file_ops		fs_file_ops;
static struct oskit_dir_ops		fs_dir_ops;
static struct oskit_absio_ops		fs_absio_ops;

/*
 * Create dir/file impls.
 */
static struct fs_file *
fs_file_allocate(const char *component)
{
        struct fs_file	*newfsf = malloc(sizeof(*newfsf));
	int		complen;

        if (newfsf == NULL)
                return 0;
	
        memset(newfsf, 0, sizeof(*newfsf));

	complen = strlen(component);
	
	if ((newfsf->component = malloc(complen + 1)) == NULL) {
		free(newfsf);
		return 0;
	}
	strcpy(newfsf->component, component);

        newfsf->count      = 1;
        newfsf->filei.ops  = &fs_file_ops;
        newfsf->absioi.ops = &fs_absio_ops;

	return newfsf;
}

static struct fs_dir *
fs_dir_allocate(const char *component)
{
        struct fs_dir	*newfsd = malloc(sizeof(*newfsd));
	int		complen;

        if (newfsd == NULL)
                return 0;
	
        memset(newfsd, 0, sizeof(*newfsd));

	complen = strlen(component);
	
	if ((newfsd->component = malloc(complen + 1)) == NULL) {
		free(newfsd);
		return 0;
	}
	strcpy(newfsd->component, component);

        newfsd->count      = 1;
        newfsd->diri.ops   = &fs_dir_ops;
        newfsd->absioi.ops = &fs_absio_ops;

	return newfsd;
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

	if (fsf->bio)
		oskit_blkio_release(fsf->bio);
	
	free(fsf->component);
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

	free(fsd->component);
	free(fsd);
	return 0;
}

/*** Operations inherited from oskit_posixio_t ***/

static OSKIT_COMDECL
fs_file_stat(oskit_file_t *f, struct oskit_stat *out_stats)
{
#if VERBOSITY > 1
        struct fs_file *fsf = (struct fs_file *) f;

	printf(__FUNCTION__": asked to stat `%s'\n", fsf->component);
#endif

	/*
	 * Can stat anything we want. Just call it a character special
	 * file and give it a nice set of permissions. 
	 */
	memset(out_stats, 0, sizeof(*out_stats));
	out_stats->mode = OSKIT_S_IFCHR|OSKIT_S_IRUSR|OSKIT_S_IWUSR|
			  OSKIT_S_IRGRP|OSKIT_S_IWGRP|
			  OSKIT_S_IROTH|OSKIT_S_IWOTH;

	return 0;
}

static OSKIT_COMDECL
fs_file_setstat(oskit_file_t *f,
		oskit_u32_t mask, const struct oskit_stat *stats)
{
#if VERBOSITY > 1
        struct fs_file  *fsf = (struct fs_file *) f;

	printf(__FUNCTION__": asked to setstat `%s'\n", fsf->component);
#endif
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_file_pathconf(oskit_file_t *f, oskit_s32_t option, oskit_s32_t *out_val)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_file_sync(oskit_file_t *f, oskit_bool_t wait)
{
#if VERBOSITY > 1
        struct fs_file  *fsf = (struct fs_file *) f;

	printf(__FUNCTION__": asked to sync on `%s'\n", fsf->component);
#endif

	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_file_access(oskit_file_t *f, oskit_amode_t mask)
{
#if VERBOSITY > 1
        struct fs_file  *fsf = (struct fs_file *) f;

	printf(__FUNCTION__": asked to access on `%s'\n", fsf->component);
#endif
	return 0;
}

static OSKIT_COMDECL
fs_file_readlink(oskit_file_t *f, char *buf, oskit_u32_t len,
		 oskit_u32_t *out_actual)
{
#if VERBOSITY > 1
        struct fs_file  *fsf = (struct fs_file *) f;

	printf(__FUNCTION__": asked to readlink on `%s'\n", fsf->component);
#endif
	*out_actual = 0;

	return OSKIT_EINVAL;
}

static OSKIT_COMDECL
fs_file_open(oskit_file_t *f, oskit_oflags_t iflags,
	     struct oskit_openfile **out_openfile)
{
        struct fs_file  *fsf = (struct fs_file *) f;
	char		*name, disk[8], part[8];
	oskit_blkio_t   *bio;

#if VERBOSITY > 1
	printf(__FUNCTION__": asked to open `%s'\n", fsf->component);
#endif
        memset(disk, 0, sizeof(disk));
        memset(part, 0, sizeof(part));

	/*
	 * Look at the component name and see if its recognizable as
	 * a linux device name. If it is, separate it from the partition
	 * and use start_disk to get it fired off.
	 */
	name = fsf->component;

	if (*name == 'r') {
		/* All we got is char devices, and linux does not like "r" */
		name++;
	}

	/*
	 * Look for sd, wd, or hd, followed by a single digit.
	 */
	if (strncmp("sd", name, 2) &&
	    strncmp("wd", name, 2) &&
	    strncmp("hd", name, 2))
		return OSKIT_EINVAL;

	if (!isdigit(name[2]))
		return OSKIT_EINVAL;

	/*
	 * Okay, got a valid disk name. Don't worry about what the partition
	 * looks like since it will be checked someplace else.
	 */
	strncpy(disk, name, 3);
	strncpy(part, &name[3], sizeof(part));

	/*
	 * Use the startup library. We get back a blkio.
	 */
	if (start_disk(disk, part,
		       (iflags & OSKIT_O_ACCMODE) == OSKIT_O_RDONLY, &bio)) {
		printf("devdir fs_file_open: "
		       "Could not start disk '%s'\n", name);
		return OSKIT_E_FAIL;
	}
	fsf->bio = bio;
	fsf->blksize = oskit_blkio_getblocksize(bio);

#if VERBOSITY > 1
	printf("devdir fs_file_open: "
	       "Opened `%s' - bio:%p blksize:%d\n", name, bio, fsf->blksize);
#endif
	/*
	 * Open succeeds, but we do not implement peropen state, so just
	 * return status. The absio interface will be used.
	 */
	*out_openfile = 0;
	return 0;
}

static OSKIT_COMDECL
fs_dir_open(oskit_dir_t *d, oskit_oflags_t iflags,
	    struct oskit_openfile **out_opendir)
{
#if VERBOSITY > 1
        struct fs_dir  *fsd = (struct fs_dir *) d;

	printf(__FUNCTION__": asked to open `%s'\n", fsd->component);
#endif
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_dir_lookup(oskit_dir_t *d, const char *name, oskit_file_t **out_file)
{
	struct fs_file  *fsf;

#if VERBOSITY > 1
        struct fs_dir	*fsd = (struct fs_dir *) d;

	printf(__FUNCTION__": asked to look up `%s' in `%s'\n",
	       name, fsd->component);
#endif

	/*
	 * Whatever it is, we got it. Its only when the device is opened
	 * that it matters.
	 *
	 * This directory contains only special files, so always create
	 * a file structure, not a directory structure. Maybe someday it
	 * will get more flexible.
	 */
	if ((fsf = fs_file_allocate(name)) == NULL)
		return OSKIT_ENOMEM;
	
	*out_file = &fsf->filei;
	return 0;
}

static OSKIT_COMDECL
fs_dir_create(oskit_dir_t *d, const char *name,
	       oskit_bool_t excl, oskit_mode_t mode,
	       oskit_file_t **out_file)
{
#if VERBOSITY > 1
        struct fs_dir	*fsd = (struct fs_dir *) d;

	printf(__FUNCTION__": asked to create `%s' in '%s'\n",
	       name, fsd->component);
#endif
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_dir_link(oskit_dir_t *d, char *name, oskit_file_t *file)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_dir_unlink(oskit_dir_t *d, const char *name)
{
#if VERBOSITY > 1
	printf(__FUNCTION__": asked to unlink `%s'\n", name);
#endif
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_dir_rename(oskit_dir_t *old_dir, char *old_name,
	      oskit_dir_t *new_dir, char *new_name)
{
#if VERBOSITY > 1
	printf(__FUNCTION__": asked to rename ...\n");
#endif
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_dir_mkdir(oskit_dir_t *d, const char *name, oskit_mode_t mode)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_dir_rmdir(oskit_dir_t *d, const char *name)
{
#if VERBOSITY > 1
	printf(__FUNCTION__": asked to rmdir `%s'\n", name);
#endif
	return OSKIT_E_NOTIMPL;
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
#if VERBOSITY > 1
        struct fs_dir		*fsd = (struct fs_dir *) d;

	printf(__FUNCTION__": asked to getdirentries `%s'\n", fsd->component);
#endif
	
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_dir_mknod(oskit_dir_t *d, char *name,
	     oskit_mode_t mode, oskit_dev_t dev)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
fs_dir_symlink(oskit_dir_t *d, char *link_name, char *dest_name)
{
#if VERBOSITY > 1
	printf(__FUNCTION__": asked to create symlink `%s' -> `%s'\n",
	       link_name, dest_name);
#endif

	return OSKIT_E_NOTIMPL;
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
 * The absio interface, intended to allow an openfile to be created
 * using an soa.
 */
static OSKIT_COMDECL
fs_afile_query(oskit_absio_t *io,
	       const struct oskit_guid *iid, void **out_ihandle)
{
        struct fs_file  *fsf = (struct fs_file *) (io - 1);
	
	if (!fsf->count)
		return OSKIT_E_INVALIDARG;

	/* may be file_query or dir_query */
	return oskit_file_query(&fsf->filei, iid, out_ihandle);
}

static OSKIT_COMDECL_U
fs_afile_addref(oskit_absio_t *io)
{
        struct fs_file  *fsf = (struct fs_file *) (io - 1);

	return oskit_file_addref(&fsf->filei);
}

static OSKIT_COMDECL_U
fs_afile_release(oskit_absio_t *io)
{
        struct fs_file  *fsf = (struct fs_file *) (io - 1);

	return oskit_file_release(&fsf->filei);
}


static OSKIT_COMDECL
fs_afile_read(oskit_absio_t *io, void *buf,
	      oskit_off_t offset, oskit_size_t amount,
	      oskit_size_t *out_actual)
{
        struct fs_file  *fsf = (struct fs_file *) (io - 1);

	if (!fsf->count)
		return OSKIT_E_INVALIDARG;

	assert(fsf->bio);
	assert((offset & (fsf->blksize - 1)) == 0);
	assert((amount & (fsf->blksize - 1)) == 0);

	return oskit_blkio_read(fsf->bio, buf, offset, amount, out_actual);
}

static OSKIT_COMDECL
fs_afile_write(oskit_absio_t *io, const void *buf,
	       oskit_off_t offset, oskit_size_t amount,
	       oskit_size_t *out_actual)
{
        struct fs_file  *fsf = (struct fs_file *) (io - 1);

	if (!fsf->count)
		return OSKIT_E_INVALIDARG;

	assert(fsf->bio);
	assert((offset & (fsf->blksize - 1)) == 0);
	assert((amount & (fsf->blksize - 1)) == 0);

	return oskit_blkio_write(fsf->bio, buf, offset, amount, out_actual);
}

static OSKIT_COMDECL
fs_afile_getsize(oskit_absio_t *io, oskit_off_t *out_size)
{
        struct fs_file  *fsf = (struct fs_file *) (io - 1);

	assert(fsf->bio);
	
	/*
	 * Ask the blockio device for its size.
	 */
	return oskit_blkio_getsize(fsf->bio, out_size);
}

static OSKIT_COMDECL
fs_afile_setsize(oskit_absio_t *io, oskit_off_t new_size)
{
#if VERBOSITY > 1
	printf(__FUNCTION__": asked to setsize\n");
#endif
	return OSKIT_E_NOTIMPL;
}

static struct oskit_absio_ops fs_absio_ops = {
	fs_afile_query,
	fs_afile_addref,
	fs_afile_release,
	(void *)0,			/* slot reserved for getblocksize */
	fs_afile_read,
	fs_afile_write,
	fs_afile_getsize,
	fs_afile_setsize
};

/*
 * Create a /dev directory in the bmod filesystem, and mount the virtual
 * directory on top of it.
 */
oskit_error_t
oskit_create_slashdev()
{
        struct fs_dir	*newfsd;
	int		rc;
	void		find_rootdisk();

	if (mkdir("/dev", 0666) < 0)
		if (errno != OSKIT_EEXIST)
			panic("oskit_create_slashdev: "
			      "Could not create /dev");

	/*
	 * Create the virtual top level directory, and mount it.
	 */
        if ((newfsd = fs_dir_allocate("/devices")) == NULL)
		panic("oskit_create_slashdev: Out of memory");		
	
	if ((rc = fs_mount("/dev", (oskit_file_t *) &newfsd->diri.ops)) != 0)
		panic("oskit_create_slashdev: fs_mount failed errno %d\n", rc);

	oskit_dir_release(&newfsd->diri);

	return 0;
}

#if 0
/*
 * Kinda silly. Look for a valid "a" partition on the first disk on
 * a bus. We call that the root partition.
 */
#define MAX_PARTS 30

void
checkforroot(oskit_blkdev_t *blk)
{
	oskit_blkio_t	*bio;
	int		numparts;
	diskpart_t	*dp, part_array[MAX_PARTS];

	if (oskit_blkdev_open(blk, OSKIT_DEV_OPEN_READ, &bio))
		return;

	/*
	 * Get the partition info.
	 */
	numparts = diskpart_blkio_get_partition(bio, part_array, MAX_PARTS);
	if (numparts == 0) {
		printf("checkforroot: No partitions found\n");
		oskit_blkio_release(bio);
		return;
	}

	diskpart_dump(part_array, 0, 'a');
}

static void
recurse_bus(oskit_bus_t *bus, char *busname, int seenroot)
{
	oskit_device_t		*dev;
	oskit_bus_t		*sub;
	oskit_devinfo_t		info;
	char			pos[OSKIT_BUS_POS_MAX+1];
	oskit_error_t		rc;
	unsigned int		i;

	for (i = 0; bus->ops->getchild(bus, i, &dev, pos) == 0; i++) {
		rc = oskit_device_getinfo(dev, &info);
		assert(rc == 0);

		rc = oskit_device_query(dev, &oskit_bus_iid, (void**)&sub);

		if (rc == 0) {
			char name[OSKIT_DEVNAME_MAX];
			
			/*
			 * This is a bus.
			 */
			strcpy(name, info.name);
			recurse_bus(sub, name, seenroot);
		} else {
			/*
			 * Just a single device.
			 */
			oskit_blkdev_t *blk;

			if (!oskit_device_query(dev, &oskit_blkdev_iid,
					(void **)&blk)) {
				checkforroot(blk);
				oskit_blkdev_release(blk);
			}
		}
		dev->ops->release(dev);
	}
}

void
find_rootdisk()
{
	char name[OSKIT_DEVNAME_MAX];

	memset(name, 0, sizeof(name));
	recurse_bus(osenv_rootbus_getbus(), name, 0);
}
#endif
