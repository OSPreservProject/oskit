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

#include <oskit/c/environment.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <alloca.h>
#include "posix.h"

/*
 * This is the interface to the filesystem namespace object. The FS
 * namespace encapsulates lookup, mount, unmount, cwd, rootdir, and
 * a few other things. The idea is keep all that stuff out of the POSIX
 * library, and in a shareable component where it belongs.
 */

#ifdef KNIT
extern oskit_fsnamespace_t	*oskit_fsnamespace;
#define fsn oskit_fsnamespace
#else
/* Actual interface object */
static oskit_fsnamespace_t	*fsn;
#endif

/* Forward prototypes */
static void			fs_release(void);

/* Initialize! */
#define INITFSN()				\
	{					\
		oskit_error_t _rc;		\
						\
		if (!fsn) {			\
			_rc = fs_init();	\
			if (_rc)		\
				return _rc;	\
		}				\
	}

/*
 * Initialize the filesystem namespace interface.
 */
static oskit_error_t
fs_init(void)
{
	static int done;

	if (done)
		return OSKIT_E_UNEXPECTED;

#ifndef KNIT
	/*
	 * Find the filesystem namespace object.
	 */
	oskit_libcenv_getfsnamespace(libc_environment, (void *) &fsn);

	if (!fsn) {
		printf("fs_init: WARNING: No Filesystem\n");
		return OSKIT_E_NOINTERFACE;
	}
#endif

	done = 1;

	/*
	 * Need to add a reference since libc_environment does not.
	 */
	oskit_fsnamespace_addref(fsn);

	/*
	 * Install an exit handler that will release the reference
	 * held by the library. This will also release all the file
	 * descriptors as well.
	 */
	atexit(fs_release);

	return 0;
}

/*
 * Shutting down. Need to release all the references so that the filesystem
 * can be sync'ed up.
 */
static void
fs_release(void)
{
	/*
	 * Release all file descriptors
	 */
	fd_cleanup();

	if (fsn)
		oskit_fsnamespace_release(fsn);
}

/*
 * Mount a filesystem.
 */
oskit_error_t
fs_mount(const char *path, oskit_file_t *file)
{
	INITFSN();

	return oskit_fsnamespace_mount(fsn, path, file);
}

/*
 * Unmount a filesystem.
 */
oskit_error_t
fs_unmount(const char *path)
{
	return oskit_fsnamespace_unmount(fsn, path);
}

/*
 * Set the current working directory. This is an internal function, called
 * from chdir and fchdir.
 */
oskit_error_t
fs_chdir(oskit_dir_t *dir)
{
	INITFSN();

	return oskit_fsnamespace_chcwd(fsn, dir);
}

/*
 * Set the current root directory. This is an internal function, called
 * from chroot.
 */
oskit_error_t
fs_chroot(const char *path)
{
	INITFSN();

	return oskit_fsnamespace_chroot(fsn, path);
}

/*
 * File lookup. Translate a path to a file object.
 */
oskit_error_t
fs_lookup(const char *name, int flags, oskit_file_t **fileret)
{
	INITFSN();

	return oskit_fsnamespace_lookup(fsn, name, flags, fileret);
}

/*
 * Lookup directory a file is in. Return a pointer to the last
 * component of the name to the caller.
 */
oskit_error_t
fs_lookup_dir(const char **name, int flags, oskit_dir_t **dirret)
{
	oskit_error_t	rc;
	oskit_file_t	*f = 0;
	char		*n;
	char		*comp;

	INITFSN();

	n = strrchr(*name, '/');
	if (!n) {
		rc = oskit_fsnamespace_lookup(fsn, ".", flags, &f);
		if (!rc)
			*dirret = (oskit_dir_t *) f;
		return 0;
	}
	n++;

	comp = alloca(n - *name + 1);
	strncpy(comp, *name, n - *name);
	comp[n - *name] = '\0';
	*name = n;

	rc = oskit_fsnamespace_lookup(fsn, comp, flags|FSLOOKUP_FOLLOW, &f);
	if (rc)
		return rc;

	/*
	 * see if it's a directory
	 */
	rc = oskit_file_query(f, &oskit_dir_iid, (void **)dirret);
	oskit_file_release(f);
	if (rc)
		rc = OSKIT_ENOTDIR;
	return rc;
}
