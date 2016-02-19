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

#include <oskit/fs/file.h>
#include <oskit/fs/openfile.h>
#include <oskit/fs/dir.h>
#include "fsn_cache.h"
#include "fsn.h"

#ifdef  FSCACHE
/* #define VERBOSE */

#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <stdio.h> 
#include "fs.h"

#ifdef  VERBOSE
#define DPRINTF(fmt, args... ) printf(__FUNCTION__  ": " fmt , ## args)
#else
#define DPRINTF(fmt, args... )
#endif

/* Maximum number of cache entries. Seems like a reasonable number ... */
#define MAX_ENTRIES	768

/* Maximum name component allowed in the cache. */
#define	MAX_CNAMELEN	30

/*
 * This is a cache entry
 */
typedef struct namecache {
	queue_chain_t	hashchain;	/* Queuing element */
	queue_chain_t	lruchain;	/* Queuing element */
	oskit_dir_t    *dir;		/* Directory this entry contained in */
	oskit_file_t   *file;		/* Actual entry. Either file or dir */
	unsigned int	hash;		/* XXX Hash value. */
	char		flags;		/* Info Flags */
	unsigned char	len;			/* Name len */
	char		name[MAX_CNAMELEN];	/* Sized for word alignment */
#ifdef VERBOSE
	int		nulls;		/* Leave room for terminating NULL */
#endif
} namecache_t;

/*
 * Notes:
 *  *  The oskit_dir_t that the entry is contained in is the unique value
 *     used to match on. The component name + dir is hashed to a chain, and
 *     then a match is found by comparing the name + dir, unitl we get an
 *     exact match.
 *  *  A minor problem with the LRU mechanism is that we are not guaranteed to
 *     get back a unique COM object for the same dir/file combination. So, 
 *     when a directory entry is tossed via LRU, it is entirely likely that
 *     its children will become orphaned and no longer useful since the next
 *     time it lands back in the cache, a different COM object might be
 *     returned, and none of the children will match. This is okay since those
 *     will eventually fall off the LRU list, and be released. The downside
 *     is that some number of objects will be retained longer (live) than they
 *     might otherwise be. The good news is that since those orphans are holding
 *     the original dir alive, we won't see that COM object for some other dir.
 *  *  Unlink (and rename) of a file requires the entry be purged. Dirs cannot
 *     be linked (good thing), so there is no worry that the same file will be
 *     in the cache twice under different directories.
 *  *  Rmdir should fail unless the directory is empty, but lets not count on
 *     that.
 *  *  Rename (and thus rmdir) require that we purge the entire cache to avoid
 *     name aliasing problems. Well, I think this is the case. 
 */

void
fsn_cache_init(struct fsnimpl *fsnimpl)
{
	struct fs_cache	*fsc = &(fsnimpl->fs_cache);
	int		 i;
	
	queue_init(&(fsc->lru));
	for (i = 0; i < NHEADERS; i++)
		queue_init(&(fsc->hash[i]));

#ifdef THREAD_SAFE
	pthread_mutex_init(&(fsc->mutex), &pthread_mutexattr_default);
#endif
}

/*
 * On exit, release all of the entries in the cache.
 */
void
fsn_cache_cleanup(struct fsnimpl *fsnimpl)
{
	struct fs_cache	*fsc = &(fsnimpl->fs_cache);
	namecache_t	*ncp;

	FSCACHE_LOCK(fsnimpl);
	DPRINTF("\n");

	/*
	 * Iterate through the LRU list (which contains every entry).
	 */
	while (!queue_empty(&fsc->lru)) {
		queue_remove_first(&fsc->lru, ncp, namecache_t *, lruchain);

		if (ncp->dir) {
			queue_remove(&fsc->hash[ncp->hash & (NHEADERS-1)],
				     ncp, namecache_t *, hashchain);

			oskit_dir_release(ncp->dir);
			oskit_file_release(ncp->file);

			DPRINTF("Cleaned: %p %p %s\n",
				ncp, ncp->dir, ncp->name);
		}
		else
			DPRINTF("Cleaned: %p\n", ncp);
			
		sfree(ncp, sizeof(*ncp));
	}
#if	defined(FSCACHE_STATS)
	printf("fscache stats:\n");
	printf("    lookups:    %d\n", fsc->stats.lookups);
	printf("    misses:     %d\n", fsc->stats.misses);
	printf("    toolong:    %d\n", fsc->stats.toolong);
	printf("    entered:    %d\n", fsc->stats.entered);
	printf("    tossed:     %d\n", fsc->stats.tossed);
	printf("    removed:    %d\n", fsc->stats.removed);
	printf("    purges:     %d\n", fsc->stats.purges);
	printf("    purged:     %d\n", fsc->stats.purged);
	printf("    renames:    %d\n", fsc->stats.renames);
	printf("    renamed:    %d\n", fsc->stats.renamed_files);
#endif
	FSCACHE_UNLOCK(fsnimpl);
}

/*
 * Enter a file (which might be a dir) into the cache. The dir is the
 * containing directory.
 *
 * NOTE: The caller is responsible for locking!
 */
void
fsn_cache_enter(struct fsnimpl *fsnimpl, oskit_dir_t *dir,
		oskit_file_t *file, char *name, int len, nameinfo_t *nameip)
{
	struct fs_cache	        *fsc = &(fsnimpl->fs_cache);
	namecache_t		*ncp;
	unsigned int		hash = 0;
	char			*cp;
	int			i;
	oskit_dir_t		*isdir;
	oskit_u32_t		flags = 0;
	oskit_error_t		rc;

	/*
	 * Determine the attributes of the file we want to add to the cache.
	 */

	/*
	 * This adds a reference! If the entry is not eventually
	 * placed in the cache, the reference MUST BE RELEASED.
	 */
	rc = oskit_file_query(file, &oskit_dir_iid, (void **)&isdir);
	if (!rc) {
		flags |= NAMEINFO_DIRECTORY;
	}
	else
		oskit_file_addref(file);

	/*
	 * If a directory, lets see if its a mountpoint. If its a file,
	 * look to see if its a symlink.
	 */
	if (!rc) {
		oskit_iunknown_t	*iu;
		struct fs_mountpoint	*mp;

		oskit_dir_query(isdir, &oskit_iunknown_iid, (void**)&iu);
		
		FSLOCK(fsnimpl);
		for (mp = fsnimpl->mountpoints; mp; mp = mp->next) {
			if (iu == mp->mountover_iu) {
				flags |= NAMEINFO_MOUNTPOINT;
				break;
			}
		}
		FSUNLOCK(fsnimpl);
		oskit_iunknown_release(iu);
	}
	else {
		char		link[1024];
		oskit_u32_t	linksize;
		
		rc = oskit_file_readlink(file, link, sizeof(link), &linksize);
		if (!rc)
			flags |= NAMEINFO_SYMLINK;
	}

	/*
	 * Return info to caller no matter what!
	 */
	nameip->flags = flags;
	nameip->file  = file;		/* To be consistent with lookup */
	
	/*
	 * Forget about names that are too long. Waste of space.
	 */
	if (len > MAX_CNAMELEN) {
		FSSTAT(fsc->stats.toolong++);
		/*
		 * Release extra reference from above.
		 */
		oskit_file_release(file);
		DPRINTF("Name too long (max=%d, is %d):  %s\n",
			MAX_CNAMELEN, len, name);
		return;
	}

	/*
	 * The hash value munges the component name with the address of
	 * the containing directory.
	 */
	for (cp = name, i = 0; i < len; i++, cp++)
		hash += (unsigned char) *cp;
	hash = hash ^ (unsigned int) dir;

	/*
	 * Grab an entry. If passed the maximum, grab the lru entry and
	 * use that (after releasing its stuff).
	 */
	if (fsc->count > MAX_ENTRIES) {
		queue_remove_first(&fsc->lru, ncp, namecache_t *, lruchain);

		DPRINTF("Toss: %p %p %s\n", ncp, ncp->dir, ncp->name);

		if (ncp->dir) {
			queue_remove(&fsc->hash[ncp->hash & (NHEADERS - 1)],
				     ncp, namecache_t *, hashchain);

			oskit_dir_release(ncp->dir);
			oskit_file_release(ncp->file);

			/*
			 * A entry is considered tossed only when its a 
			 * capacity miss (not purged or removed).
			 */
			if (ncp->len != (unsigned char) -1)
				FSSTAT(fsc->stats.tossed++);
		}
	}
	else {
		if ((ncp = (namecache_t *) smalloc(sizeof(namecache_t)))
		    == NULL) {
			/*
			 * Release extra reference from above. 
			 */
			oskit_file_release(file);
			return;
		}
		memset((void *) ncp, 0, sizeof(namecache_t));
		fsc->count++;
	}

	/*
	 * Initialize entry and place it in its bucket. Note that we must add
	 * a references to the dir object. The file object had a reference
	 * added above.
	 */
	ncp->hash   = hash;
	ncp->dir    = dir;
	ncp->file   = file;
	ncp->len    = len;
	ncp->flags  = flags;
	memcpy(ncp->name, name, len);
#ifdef  VERBOSE
	ncp->name[len] = '\0';
#endif
	oskit_dir_addref(dir);

	/*
	 * Link into the hash chain.
	 */
	queue_enter(&fsc->hash[hash & (NHEADERS - 1)],
		    ncp, namecache_t *, hashchain);

	/*
	 * New entries go on the LRU chain
	 */
	queue_enter(&fsc->lru, ncp, namecache_t *, lruchain);

	FSSTAT(fsc->stats.entered++);
	DPRINTF("New:  %p %p %p %s\n", ncp, dir, file, ncp->name);
}

/*
 * NOTE: The caller is responsible for locking!
 */
oskit_error_t
fsn_cache_lookup(struct fsnimpl *fsnimpl,
		 oskit_dir_t *dir, char *name, int len, nameinfo_t *nameip)
{
	struct fs_cache *fsc = &(fsnimpl->fs_cache);
	namecache_t	*ncp;
	unsigned int	hash = 0;
	char		*cp;
	int		i;

	FSSTAT(fsc->stats.lookups++);

	/*
	 * Forget about names that are too long. Waste of space.
	 */
	if (len > MAX_CNAMELEN) {
		FSSTAT(fsc->stats.toolong++);
		nameip->flags = NULL;
		nameip->file  = NULL;
		return 0;
	}

	/*
	 * The hash value munges the component name with the address of
	 * the containing directory.
	 */
	for (cp = name, i = 0; i < len; i++, cp++)
		hash += (unsigned char) *cp;
	hash = hash ^ (unsigned int) dir;

	/*
	 * Iterate through the hash chain. Compare both the name and
	 * the dir object.
	 */
	queue_iterate(&fsc->hash[hash & (NHEADERS - 1)],
		      ncp, namecache_t *, hashchain) {
		if (ncp->dir == dir && ncp->len == len &&
		    memcmp(ncp->name, name, len) == 0)
			goto found;
	}
	FSSTAT(fsc->stats.misses++);
	return 0;

 found:
	DPRINTF("%p %p %s\n", ncp, dir, ncp->name);

	/*
	 * Return info to caller.
	 */
	nameip->flags = ncp->flags;
	nameip->file  = ncp->file;
	oskit_file_addref(ncp->file);
		
	/*
	 * Move to head of LRU chain.
	 */
	queue_remove(&fsc->lru, ncp, namecache_t *, lruchain);
	queue_enter(&fsc->lru, ncp, namecache_t *, lruchain);

	return 1;
}

/*
 * Find and remove a particular entry from the cache. Used when a file is
 * unlinked.
 *
 * NOTE: The caller is responsible for locking!
 */
void
fsn_cache_remove(struct fsnimpl *fsnimpl,
		 oskit_dir_t *dir, const char *name)
{
	struct fs_cache *fsc = &(fsnimpl->fs_cache);
	namecache_t	*ncp;
	int		len;
	unsigned int	hash = 0;
	char		*cp;

	len  = strlen(name);

	/*
	 * Forget about names that are too long. Waste of space.
	 */
	if (len > MAX_CNAMELEN) {
		FSSTAT(fsc->stats.toolong++);
		return;
	}

	/*
	 * The hash value munges the component name with the address of
	 * the containing directory.
	 */
	for (cp = (char *) name; *cp != 0; cp++)
		hash += (unsigned char) *cp;
	hash = hash ^ (unsigned int) dir;

	/*
	 * Iterate through the hash chain. Compare both the name and
	 * the dir object.
	 */
	queue_iterate(&fsc->hash[hash & (NHEADERS - 1)],
		      ncp, namecache_t *, hashchain) {
		if (ncp->dir == dir && ncp->len == len &&
		    strcmp(ncp->name, name) == 0)
			goto found;
	}
	return;

 found:
	DPRINTF("%p %p %s\n", ncp, dir, name);
	FSSTAT(fsc->stats.removed++);

	oskit_file_release(ncp->file);
	oskit_dir_release(ncp->dir);

	/*
	 * Leave the entry on the LRU list. It will eventually fall to the
	 * bottom and get reused. Clear the dir/file entries to indicate
	 * that they have been released already, and that the entry is no
	 * longer on the hash chain.
	 */
	ncp->dir    = 0;
	ncp->file   = 0;
	
	queue_remove(&fsc->hash[hash & (NHEADERS - 1)],
		     ncp, namecache_t *, hashchain);
}

/*
 * Purge the cache of all entries when a directory is either renamed or
 * removed.  We do not release the entries (like remove above) since that
 * would penalize the caller (for the time to flush the entire cache).
 * Rather, force subsequent matches to fail, which spreads the cost of
 * releasing every entry among all the callers.
 *
 * NOTE: The caller is responsible for locking!
 */
void
fsn_cache_purge(struct fsnimpl *fsnimpl)
{
	struct fs_cache	*fsc = &(fsnimpl->fs_cache);
	namecache_t	*ncp;

	DPRINTF("\n");
	FSSTAT(fsc->stats.purges++);

	/*
	 * Iterate through the LRU list (which contains every entry).
	 */
	queue_iterate(&fsc->lru, ncp, namecache_t *, lruchain) {
		/*
		 * Force match to fail.
		 */
		if (ncp->len != (unsigned char) -1) {
			ncp->len = -1;
			FSSTAT(fsc->stats.purged++);
		}
	}
}

void
fsn_cache_rename(struct fsnimpl *fsnimpl,
		 oskit_dir_t *olddir, const char *oldname,
		 oskit_dir_t *newdir, const char *newname)
{
	oskit_error_t	rc;
	oskit_file_t	*file;
	oskit_dir_t	*isdir;
#ifdef FSCACHE_STATS
	struct fs_cache *fsc = &(fsnimpl->fs_cache);
#endif

	DPRINTF("%p %s, %p %s\n", olddir, oldname, newdir, newname);
	FSSTAT(fsc->stats.renames++);
	
	/*
	 * Look to see if the target is a file or a directory.
	 */
	rc = oskit_dir_lookup(newdir, newname, &file);
	if (rc) {
		return fsn_cache_purge(fsnimpl);
	}

	/* 
	 * see if it's a directory. Just purge the whole thing if it is.
	 */
	rc = oskit_file_query(file, &oskit_dir_iid, (void **)&isdir);
	if (!rc) {
		oskit_dir_release(isdir);
		oskit_file_release(file);
		return fsn_cache_purge(fsnimpl);
	}

	/*
	 * Its a file instead. We can purge just those entries that refer
	 * to the old and new files.
	 */
	FSSTAT(fsc->stats.renamed_files++);
	fsn_cache_remove(fsnimpl, olddir, oldname);
	fsn_cache_remove(fsnimpl, newdir, newname);
	oskit_file_release(file);
}

/*
 * A slightly different entrypoint for the fs_lookup code.
 *
 * NOTE: The caller is *NOT* responsible for locking!
 */
void
fsn_cache_delete(struct fsnimpl *fsnimpl,
		 oskit_dir_t *dir, const char *name, int len)
{
	char		namecopy[MAX_CNAMELEN + 1];

	/*
	 * Forget about names that are too long. Waste of space.
	 */
	if (len > MAX_CNAMELEN)
		return;

	memcpy(namecopy, name, len);
	namecopy[len] = NULL;
	
	FSCACHE_LOCK(fsnimpl);
	fsn_cache_remove(fsnimpl, dir, (const char *) namecopy);
	FSCACHE_UNLOCK(fsnimpl);
}
#endif /* FSCACHE */
