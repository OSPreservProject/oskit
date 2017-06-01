/*
 * Copyright (c) 1999, 2000 The University of Utah and the Flux Group.
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
 * Create a memfs and populate it with the bmod contents. Return the
 * memfs root directory, but *do not* hand it over to the clientos.
 */
#include <stdio.h>
#include <stdlib.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/memfs.h>
#include <oskit/startup.h>
#include <oskit/config.h>

#ifdef PTHREADS
/*
 * Using the process lock on a BMOD may seem silly.
 * The bmod code had the locking ripped out, and this is easier than
 * sticking it back in. In any event, the process lock is just a mutex,
 * and thats what the old bmod locking did, so its almost a wash.
 */
#include <oskit/com/wrapper.h>
#include <oskit/threads/pthread.h>

void		fs_master_lock(void);
void		fs_master_unlock(void);
#define	start_bmod	start_bmod_pthreads
#endif

oskit_dir_t *
start_bmod()
{
	oskit_filesystem_t *memfs;
	oskit_dir_t        *root;
	int 	rc;

#ifdef KNIT
	rc = oskit_memfs_init(&memfs);
#else
	rc = oskit_memfs_init(start_osenv(), &memfs);
#endif
	if (rc) {
		printf("start_bmod:  Could not allocate a bmod!\n");
		return 0;
	}

	rc = oskit_filesystem_getroot(memfs, &root);
	if (rc) {
		printf("start_bmod:  Could not getroot the bmod!\n");
		return 0;
	}
	oskit_filesystem_release(memfs);

	bmod_populate(root, NULL, NULL);

#ifdef PTHREADS
	{
		oskit_dir_t	*wrappeddir;
		int		rc;

		/* Wrap it up! */
		rc = oskit_wrap_dir(root,
				(void (*)(void *))fs_master_lock,
				(void (*)(void *))fs_master_unlock,
				0, &wrappeddir);

		if (rc) {
			printf("oskit_wrap_dir() failed: errno 0x%x\n", rc);
			exit(rc);
		}

		/* Don't need the dir anymore, the wrapper has a ref. */
		oskit_dir_release(root);
		root = wrappeddir;
	}
#endif
	return root;
}
