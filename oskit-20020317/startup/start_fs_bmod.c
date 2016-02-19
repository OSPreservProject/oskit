/*
 * Copyright (c) 1999 The University of Utah and the Flux Group.
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
 * Create a memfs and populate it with the bmod contents. Hand the
 * root directory of the memfs over to the clientos to install as
 * the root directory of the filesystem.
 */
#include <oskit/fs/dir.h>
#include <oskit/fs/memfs.h>
#include <oskit/fs/fsnamespace.h>
#include <stdio.h>
#include <stdlib.h>
#include <oskit/startup.h>
#include <oskit/clientos.h>

static void		start_fs_bmod_cleanup(void *arg);

#ifdef PTHREADS
#define	start_bmod	start_bmod_pthreads
#define	start_fs_bmod	start_fs_bmod_pthreads
#endif

void
start_fs_bmod()
{
	oskit_dir_t		*memfs = start_bmod();
	oskit_fsnamespace_t	*fsn;

	if (!memfs) {
		printf("start_fs_bmod: "
		       "Could not initialize the bmod filesystem!\n");
		exit(69);
	}

	/*
	 * Create the filesystem namespace and set it.
	 */
	if (oskit_create_fsnamespace(memfs, memfs, &fsn)) {
		printf("start_fs_bmod: "
		       "Could not create filesystem namespace!\n");
		exit(69);
	}

	/*
	 * Release our reference since the fsnamespace took a couple.
	 */
	oskit_dir_release(memfs);

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
	startup_atexit(start_fs_bmod_cleanup, NULL);
}

/*
 * Must release the filesystem namespace upon exit so that everything
 * gets cleared away. Of course, this is not strictly required in a
 * memory filesystem, but this code is supposed to be demonstration code,
 * so might as well be complete.
 */
static void
start_fs_bmod_cleanup(void *arg)
{
	oskit_clientos_setfsnamespace(NULL);
}
