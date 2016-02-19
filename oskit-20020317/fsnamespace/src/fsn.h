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

#ifndef _OSKIT_FSNAMESPACE_FSN_H_
#define _OSKIT_FSNAMESPACE_FSN_H_

#include <oskit/fs/fsnamespace.h>
#include "fsn_cache.h"
#ifdef THREAD_SAFE
#include <oskit/threads/pthread.h>
#endif

/*
 * The underlying fs namespace structure. This is shared among all
 * users of the fsnamespace object, via the COM object. That is, there is
 * one shared namespace structure that contains all the actual stuff.
 * Each fsnamespace COM object refers to that, albiet with a different
 * root and cwd.
 */
struct fsnimpl {
	int			count;		/* This is reference counted */
	struct fs_mountpoint   *mountpoints;    /* Mountpoint list */
#ifdef  THREAD_SAFE
	pthread_mutex_t		mutex;		/* Protect impl */
#endif
#ifdef  FSCACHE
	struct fs_cache		fs_cache;	/* FS cache stuff */
#endif
};

/*
 * The fsnamespace COM object. We pass this into lookup and lookup_dir
 * so that it can access the impl and croot/cwd.
 */
struct fsobj {
	oskit_fsnamespace_t     fsni;		/* The COM interface */
	int			count;		/* Reference count */
	struct fsnimpl	       *fsnimpl;	/* Pointer to namespace impl */
	oskit_dir_t	       *croot;		/* Current root directory */
	oskit_dir_t	       *cwd;		/* Current working directory */
#ifdef  THREAD_SAFE
	pthread_mutex_t		mutex;		/* Protect FS object */
#endif
};

/*
 * List of mount points and the nodes mounted on them.
 */
struct fs_mountpoint {
	oskit_file_t		*subtree;	/* dir/file to mount */
	oskit_iunknown_t	*subtree_iu;	/* iunknown intf of above */
	oskit_dir_t		*mountover;	/* dir to mount it on */
	oskit_iunknown_t	*mountover_iu;	/* iunknown intf of above */
	struct fs_mountpoint	*next;
};

/*
 * Thread safe version needs locking!
 */
#ifdef THREAD_SAFE
#define FSLOCK(f)		pthread_mutex_lock(&((f)->mutex))
#define FSUNLOCK(f)		pthread_mutex_unlock(&((f)->mutex))
#else
#define FSLOCK(f)
#define FSUNLOCK(f)
#endif

/* External prototypes for fs internal operations. */
oskit_error_t	fsn_lookup(struct fsobj *,
			const char *, int, int *, oskit_file_t **);
oskit_error_t	fsn_lookup_dir(struct fsobj *,
			const char *, int, char **, oskit_dir_t **);
oskit_error_t	fsn_mount(struct fsnimpl *, oskit_dir_t *, oskit_file_t *);
oskit_error_t	fsn_unmount(struct fsnimpl *, oskit_file_t *);
oskit_error_t   fsn_wrap_dir(struct fsnimpl *fsnimpl,
			oskit_dir_t *in, oskit_dir_t **out);
oskit_error_t	fsn_unwrap_dir(oskit_dir_t *in, oskit_dir_t **out);

#endif /* _OSKIT_FSNAMESPACE_FSN_H_ */
