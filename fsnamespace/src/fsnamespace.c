/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
#include <oskit/fs/dir.h>
#include <oskit/fs/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include "fsn.h"

/* Forward decl */
static struct oskit_fsnamespace_ops	fsnamespace_ops;

/* For debugging, lets keep a handle on the initial objects. */
static struct fsobj	*initial_fsn;
static struct fsnimpl	*initial_fsnimpl;

static OSKIT_COMDECL 
fsnamespace_query(oskit_fsnamespace_t *f,
		  const oskit_iid_t *iid, void **out_ihandle)
{ 
	struct fsobj	*fs = (struct fsobj *) f;

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_fsnamespace_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &fs->fsni;
		FSLOCK(fs);
		fs->count++;
		FSUNLOCK(fs);
                return 0; 
        }
  
        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
fsnamespace_addref(oskit_fsnamespace_t *f)
{
	struct fsobj	*fs = (struct fsobj *) f;
	int		newcount;

	assert(fs->count);
	FSLOCK(fs);
	newcount = ++fs->count;
	FSUNLOCK(fs);

	return newcount;
}

static OSKIT_COMDECL_U
fsnamespace_release(oskit_fsnamespace_t *f)
{
	struct fsobj	*fs = (struct fsobj *) f;
	int		newcount;

	assert(fs->count);

	FSLOCK(fs);
	newcount = --fs->count;
	FSUNLOCK(fs);

	if (newcount == 0) {
#ifdef  FSCACHE
		fsn_cache_cleanup(fs->fsnimpl);
#endif
		oskit_dir_release(fs->cwd);
		oskit_dir_release(fs->croot);
		sfree(fs, sizeof(*fs));
	}
	return newcount;
}

/*
 * Change the root directory. Subsequent absolute path lookups
 * will be rooted at the new directory.
 */
static OSKIT_COMDECL
fsnamespace_chroot(oskit_fsnamespace_t *f, const char *dirname)
{
	struct fsobj	*fs = (struct fsobj *) f;
	oskit_dir_t	*oroot, *dir;
	oskit_file_t	*file;
	oskit_error_t	rc;
	int		isdir;
	
	assert(fs->count);

	/*
	 * Do a pathname lookup using the current root and cwd.
	 */
	rc = fsn_lookup(fs, dirname, FSLOOKUP_FOLLOW, &isdir, &file);
	if (rc)
		return rc;
	
	rc = oskit_file_query(file, &oskit_dir_iid, (void **) &dir);
	oskit_file_release(file);
	if (rc)
		return (rc == OSKIT_E_NOINTERFACE) ?
			OSKIT_ENOTDIR : OSKIT_EINVAL;

	/*
	 * This is not multithread safe, but is set up so that the lookup
	 * code will not get into trouble. But, the caller should probably
	 * know what its doing if it is going to change the root directory
	 * in a multithreaded program.
	 */
	oroot = fs->croot;
#ifdef	FSCACHE
	/*
	 * We don't want to use the fscache wrapper here.
	 */
	rc = fsn_unwrap_dir(dir, &fs->croot); assert(rc == 0);
	
	/*
	 * Release ref above, since we don't use it. Instead, unwrap
	 * added a ref to the wrapped dir before it returned it.
	 */
	oskit_dir_release(dir);
#else
	fs->croot = dir;
#endif
	if (oroot)
		oskit_dir_release(oroot);
	
	return 0;
}

/*
 * Change the current working directory (cwd). Subsequent relative
 * path lookups will start here. 
 */
static OSKIT_COMDECL
fsnamespace_chcwd(oskit_fsnamespace_t *f, struct oskit_dir *cwd)
{
	struct fsobj	*fs = (struct fsobj *) f;
	oskit_dir_t	*ocwd, *dir;
	oskit_error_t	rc;

	assert(fs->count);

	rc = oskit_dir_query(cwd, &oskit_dir_iid, (void **) &dir);
	if (rc)
		return OSKIT_ENOTDIR;

	/*
	 * This is not multithread safe, but is set up so that the lookup
	 * code will not get into trouble. But, the caller should probably
	 * know what its doing if it is going to change the cwd in a
	 * multithreaded program.
	 */
	ocwd = fs->cwd;
#ifdef	FSCACHE
	/*
	 * We don't want to use the fscache wrapper here.
	 */
	rc = fsn_unwrap_dir(dir, &fs->cwd); assert(rc == 0);

	/*
	 * Release ref above, since we don't use it. Instead, unwrap
	 * added a ref to the wrapped dir before it returned it.
	 */
	oskit_dir_release(dir);
#else
	fs->cwd = dir;
#endif
	if (ocwd)
		oskit_dir_release(ocwd);

	return 0;
}

/*
 * Lookup a name in the filesystem. Standard name translation,
 * subject to flags described below. 
 */
static OSKIT_COMDECL
fsnamespace_lookup(oskit_fsnamespace_t *f, const char *filename,
		   int flags, struct oskit_file **out_file)
{
	struct fsobj	*fs = (struct fsobj *) f;
	oskit_file_t	*file;
	oskit_error_t	rc;
	int		isdir;

	assert(fs->count);

	/*
	 * Do a pathname lookup using the current root and cwd.
	 */
	rc = fsn_lookup(fs, filename, flags, &isdir, &file);
	if (rc)
		return rc;

#ifdef	FSCACHE
	/*
	 * Must wrap up directories so that cache consistency can be
	 * maintained. See the dir_wrapper code.  Fortunately, most
	 * operations are on files, not directories.
	 */
	if (isdir) {
		oskit_dir_t	*dir;

		rc = fsn_wrap_dir(fs->fsnimpl, (oskit_dir_t *) file, &dir);
		
		/*
		 * Release reference since the wrapper took a ref.
		 */
		oskit_file_release(file);
		
		if (rc) 
			return rc;

		file = (oskit_file_t *) dir;
	}
#endif
	*out_file = file;
	return 0;
}
		
/*
 * Create a mountpoint in the filesystem. A directory subtree is
 * mounted over the directory named by mountdir. A lookup operation
 * is performed on mountdir to find the mounted over oskir_dir_t.
 */
static OSKIT_COMDECL
fsnamespace_mount(oskit_fsnamespace_t *f,
		  const char *mountdir, struct oskit_file *subtree)
{
	struct fsobj	*fs = (struct fsobj *) f;
	oskit_dir_t	*dir;
	oskit_file_t	*file;
	oskit_error_t	rc;
	int		isdir;

	assert(fs->count);

	/*
	 * Lookup the pathname and make sure its a directory.
	 */
	rc = fsn_lookup(fs, mountdir,
			FSLOOKUP_FOLLOW|FSLOOKUP_NOCACHE, &isdir, &file);
	if (rc)
		return rc;

	rc = oskit_file_query(file, &oskit_dir_iid, (void **)&dir);
	oskit_file_release(file);
	if (rc)
		return rc == OSKIT_E_NOINTERFACE ? OSKIT_ENOTDIR : rc;

	rc = fsn_mount(fs->fsnimpl, dir, subtree);
	if (rc)
		oskit_dir_release(dir);

	return rc;
}

/*
 * Undo a mountpoint in the filesystem. 
 */
static OSKIT_COMDECL
fsnamespace_unmount(oskit_fsnamespace_t *f, const char *mountpoint)
{
	struct fsobj	*fs = (struct fsobj *) f;
	oskit_file_t	*file;
	oskit_error_t	rc;
	int		isdir;

	assert(fs->count);

	/*
	 * Lookup the pathname.
	 */
	rc = fsn_lookup(fs, mountpoint,
			FSLOOKUP_FOLLOW|FSLOOKUP_NOCACHE, &isdir, &file);
	if (rc)
		return rc;

	return fsn_unmount(fs->fsnimpl, file);
}

/*
 * Clone an fsnamespace object. Typically used to give a child
 * its own view of the filesystem namespace (root/cwd). All else
 * stays the same of course.
 */
static OSKIT_COMDECL
fsnamespace_clone(oskit_fsnamespace_t *f,
		  oskit_fsnamespace_t **out_fsnamespace)
{
	struct fsobj		*fs = (struct fsobj *) f;
	struct fsobj		*newfs;

	newfs = (struct fsobj *) smalloc(sizeof(*newfs));
	if (newfs == NULL)
		return OSKIT_E_OUTOFMEMORY;
	memset(newfs, 0, sizeof(*newfs));

	FSLOCK(fs);
	newfs->fsni.ops  = &fsnamespace_ops;
	newfs->count     = 1;
	newfs->croot     = fs->croot;
	oskit_dir_addref(newfs->croot);
	newfs->cwd       = fs->cwd;
	oskit_dir_addref(newfs->cwd);
	newfs->fsnimpl   = fs->fsnimpl;
	FSUNLOCK(fs);
	
	FSLOCK(fs->fsnimpl);
	fs->fsnimpl->count++;
	FSUNLOCK(fs->fsnimpl);

	*out_fsnamespace = &newfs->fsni;

	return 0;
}

static struct oskit_fsnamespace_ops fsnamespace_ops = {
	fsnamespace_query,
	fsnamespace_addref,
	fsnamespace_release,
	fsnamespace_chroot,
	fsnamespace_chcwd,
	fsnamespace_lookup,
	fsnamespace_mount,
	fsnamespace_unmount,
	fsnamespace_clone,
};

oskit_error_t
oskit_create_fsnamespace(struct oskit_dir *rootdir,
			 struct oskit_dir *cwd,
			 oskit_fsnamespace_t **out_fsnamespace)
{
	struct fsobj	*newfs;
	struct fsnimpl  *newimpl;

	newfs = (struct fsobj *) smalloc(sizeof(*newfs));
	if (newfs == NULL)
		return OSKIT_E_OUTOFMEMORY;
	memset(newfs, 0, sizeof(*newfs));

	newimpl = (struct fsnimpl *) smalloc(sizeof(*newimpl));
	if (newimpl == NULL) {
		sfree(newfs, sizeof(*newfs));
		return OSKIT_E_OUTOFMEMORY;
	}
	memset(newimpl, 0, sizeof(*newimpl));

	newfs->fsni.ops  = &fsnamespace_ops;
	newfs->count     = 1;
	newfs->croot     = rootdir;
	oskit_dir_addref(newfs->croot);
	newfs->cwd       = cwd;
	oskit_dir_addref(newfs->cwd);

	newfs->fsnimpl   = newimpl;
	newimpl->count   = 1;

	/* Debugging */
	initial_fsn      = newfs;
	initial_fsnimpl  = newimpl;

#ifdef  FSCACHE
	fsn_cache_init(newimpl);
#endif
	*out_fsnamespace = &newfs->fsni;
	return 0;
}

