/*
 * Copyright (c) 1997-2001 University of Utah and the Flux Group.
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

#include <oskit/startup.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/fs/linux.h>
#include <oskit/fs/filesystem.h>
#include <oskit/fs/file.h>
#include <oskit/fs/dir.h>
#include <oskit/io/blkio.h>
#include <oskit/diskpart/diskpart.h>
#include <oskit/principal.h>
#include <oskit/boolean.h>
#include <oskit/clientos.h>

#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/unistd.h>
#include <oskit/c/fs.h>
#include <oskit/c/fd.h>

#ifdef PTHREADS
#include <oskit/com/wrapper.h>
#include <oskit/threads/pthread.h>

#define	start_linux_fs		start_linux_fs_pthreads
#define	start_linux_fs_on_blkio	start_linux_fs_on_blkio_pthreads
#endif

extern int oskit_usermode_simulation;

/*
 * Return error and let caller supply 0 default.
 */
oskit_error_t
oskit_get_call_context(const struct oskit_guid *iid, void **out_if)
{
    *out_if = 0;
    return OSKIT_E_NOINTERFACE;
}

/*
 * Must release the filesystem namespace upon exit so that everything
 * gets cleared away. With all the references cleared, the actual
 * filesystem can by sync'ed to disk, and then it can be released!
 */
static void
start_linux_fs_cleanup(void *arg)
{
    oskit_filesystem_t *fs = arg;
    int			count;

    oskit_clientos_setfsnamespace(NULL);
    printf("Syncing disks ... ");
    oskit_filesystem_sync(fs, 1);
    printf("Done!\n");
    if ((count = oskit_filesystem_release(fs)) != 0) {
	    printf("WARNING: start_linux_fs_cleanup: "
		   "filesystem not RELEASED(%d)!\n", count);
    }
}

void
start_linux_fs_on_blkio(oskit_blkio_t *part)
{
    oskit_dir_t *root;
    oskit_filesystem_t *fs;
    oskit_fsnamespace_t	*fsn;
    int rc;

    osenv_process_lock();

    /* initialize the file system code */
    start_osenv();
    rc = fs_linux_init();
    if (rc)
    {
        printf("fs_linux_init() failed:  errno 0x%x\n", rc);
        exit(rc);
    }

    /* mount a filesystem */
    rc = fs_linux_mount(part, 0, &fs);
    if (rc)
    {
	printf("fs_linux_mount() failed:  errno 0x%x\n", rc);
	exit(rc);
    }

    if (!oskit_usermode_simulation) {
	/* Don't need the part anymore, the filesystem has a ref. */
	oskit_blkio_release(part);
    }

#ifdef PTHREADS
    {
	oskit_filesystem_t	*wrappedfs;

	/* Wrap it up! */
	rc = oskit_wrap_filesystem(fs,
			(void (*)(void *))osenv_process_lock,
			(void (*)(void *))osenv_process_unlock,
			0, &wrappedfs);

	if (rc)
	{
	    printf("oskit_wrap_filesystem() failed: errno 0x%x\n", rc);
	    exit(rc);
	}

	/* Don't need the fs anymore, the wrapper has a ref. */
	oskit_filesystem_release(fs);
	fs = wrappedfs;

	/*
	 * oskit_filesystem_getroot will return a wrapped root dir.
	 */
    }
#endif

    osenv_process_unlock();
    
    rc = oskit_filesystem_getroot(fs, &root);
    if (rc)
    {
	printf("oskit_filesystem_getroot() failed: errno 0x%x\n", rc);
	exit(rc);
    }

    /* Create the initial filesystem namespace from the root directory */
    rc = oskit_create_fsnamespace(root, root, &fsn);
    if (rc)
    {
	printf("oskit_create_fsnamespace() failed: errbo 0x%x\n", rc);
	exit(rc);
    }

    /* Don't need the root anymore, fsnamespace took a couple of refs. */
    oskit_dir_release(root);

    /* Initialize the filesystem namespace for the program. */
    oskit_clientos_setfsnamespace(fsn);

    /* And release our reference since the clientos took one. */
    oskit_fsnamespace_release(fsn);

    /* Set up to clear out the clientos fsname handle at exit. */
    startup_atexit(start_linux_fs_cleanup, fs);
}

void
start_linux_fs(const char *diskname, const char *partname)
{
	int rc;
	oskit_blkio_t *part;

	osenv_process_lock();
	rc = start_disk(diskname, partname, 0, &part);
	osenv_process_unlock();
	if (rc) {
		printf("start_disk() failed:  ernno 0x%x\n", rc);
		exit(rc);
	}

	start_linux_fs_on_blkio(part);
}
