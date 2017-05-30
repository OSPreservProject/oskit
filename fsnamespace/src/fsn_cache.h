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

#ifndef _OSKIT_FSNAMESPACE_FSCACHE_H_
#define _OSKIT_FSNAMESPACE_FSCACHE_H_

#include <oskit/fs/file.h>
#include <oskit/fs/dir.h>
#include <oskit/queue.h>

/*
 * This turns on the FS cache.
 */
#define FSCACHE

/*
 * Turn this on if you want some stats.
 */
#undef FSCACHE_STATS

/*
 * Number of hash chain headers.
 */
#define NHEADERS		128	/* Must be a power of two! */

/*
 * This structure is returned from fs_cache_lookup and enter.
 */
typedef struct {
	oskit_file_t	*file;		/* File or dir */
	oskit_u32_t	flags;		/* Info flags */
} nameinfo_t;
#define NAMEINFO_DIRECTORY	0x01
#define NAMEINFO_MOUNTPOINT	0x02
#define NAMEINFO_SYMLINK	0x04

#ifdef THREAD_SAFE
#include <oskit/threads/pthread.h>
#endif

/*
 * Build against thread library directly, instead of indirecting through
 * the lock manager.
 */
#if defined(THREAD_SAFE) && defined(FSCACHE)
#define FSCACHE_LOCK(f)		pthread_mutex_lock(&((f)->fs_cache.mutex))
#define FSCACHE_UNLOCK(f)	pthread_mutex_unlock(&((f)->fs_cache.mutex))
#else
#define FSCACHE_LOCK(f)
#define FSCACHE_UNLOCK(f)
#endif

#ifdef  FSCACHE_STATS
struct fscache_stats {
	oskit_u32_t	lookups;
	oskit_u32_t	misses;
	oskit_u32_t	toolong;
	oskit_u32_t	entered;
	oskit_u32_t	tossed;
	oskit_u32_t	removed;
	oskit_u32_t	purges;
	oskit_u32_t	purged;
	oskit_u32_t	renames;
	oskit_u32_t	renamed_files;
};
#define FSSTAT(x) (x)
#else
#define FSSTAT(x)
#endif

/*
 * Okay, this is the structure that goes into the fsimpl
 */
struct fs_cache {
	queue_head_t		hash[NHEADERS]; /* Hash chain headers */
	queue_head_t		lru;		/* LRU list for replacement */
	int			count;		/* Number of entries */
#ifdef  THREAD_SAFE
	pthread_mutex_t		mutex;
#endif
#ifdef  FSCACHE_STATS
	struct fscache_stats    stats;
#endif
};

struct fsnimpl;

void		fsn_cache_init(struct fsnimpl *);
void		fsn_cache_cleanup(struct fsnimpl *);
void		fsn_cache_enter(struct fsnimpl *, oskit_dir_t *,
				oskit_file_t *, char *, int, nameinfo_t *);
oskit_error_t   fsn_cache_lookup(struct fsnimpl *,
				oskit_dir_t *, char *, int, nameinfo_t *);
void		fsn_cache_remove(struct fsnimpl *,
				oskit_dir_t *dir, const char *name);
void		fsn_cache_delete(struct fsnimpl *,
				oskit_dir_t *dir, const char *name, int len);
void		fsn_cache_purge(struct fsnimpl *);
void		fsn_cache_rename(struct fsnimpl *,
				oskit_dir_t *, const char *,
				oskit_dir_t *, const char *);

#endif /* _OSKIT_FSNAMESPACE_FSCACHE_H_ */
