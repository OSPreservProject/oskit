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
 * start_fs_native.c
 *
 * Start up native filesystem.
 */
#include <oskit/dev/error.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/fsnamespace.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>
#include <oskit/c/assert.h>

extern oskit_dir_t     *oskit_unix_fs_init(char *rootname);
static void		start_fs_native_cleanup(void *arg);

void
start_fs_native(char *rootname)
{
	oskit_dir_t		*root = oskit_unix_fs_init(rootname);
	oskit_fsnamespace_t	*fsn;

	if (!root) {
		printf("start_fs_native: "
		       "Could not initialize the native filesystem!\n");
		exit(69);
	}

	/*
	 * Create the filesystem namespace and set it.
	 */
	if (oskit_create_fsnamespace(root, root, &fsn)) {
		printf("start_fs_native: "
		       "Could not create filesystem namespace!\n");
		exit(69);
	}

	/*
	 * Release our reference since the fsnamespace took a couple.
	 */
	oskit_dir_release(root);

	/*
	 * Initialize the filesystem namespace for the program.
	 */
	oskit_clientos_setfsnamespace(fsn);

	/*
	 * And release our reference since the clientos took one.
	 */
	oskit_fsnamespace_release(fsn);

	/*
	 * Set up to clear out the clientos fsname handle at exit.
	 */
	startup_atexit(start_fs_native_cleanup, NULL);
}

/*
 * Must release the filesystem namespace upon exit so that everything
 * gets cleared away. Of course, this is not strictly required, but this
 * code is supposed to be demonstration code, so might as well be complete.
 */
static void
start_fs_native_cleanup(void *arg)
{
	oskit_clientos_setfsnamespace(NULL);
}
