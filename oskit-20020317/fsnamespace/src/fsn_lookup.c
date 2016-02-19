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
 * lookup a file or directory
 */

#include <oskit/com.h>
#include <oskit/fs/file.h>
#include <oskit/fs/dir.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "fsn.h"

#define MAXPATHLENGTH	1024
#define MAXSYMLINKDEPTH	20

#define VERBOSITY	0
#ifndef VERBOSITY
#define VERBOSITY	0
#endif

#define GETPARENT() \
	if (wantparent) { \
	}

/*
 * Look up file with name name relative to dir base.
 */
oskit_error_t
fsn_lookup(struct fsobj *fs,
	   const char *name, int flags, int *isdir, oskit_file_t **fileret)
{
	oskit_error_t	rc;
	oskit_dir_t	*dir;
	oskit_file_t	*file;
	char		wname[MAXPATHLENGTH];
	char		*comp;
	char	 	*s;
	char		savechar;
	char		link[MAXPATHLENGTH];
	oskit_u32_t	linksize;
	int		symlinkdepth = 0;
	struct fs_mountpoint *mp;
	oskit_iunknown_t	*iu;
	char		wantparent = flags & FSLOOKUP_PARENT;
	struct fsnimpl  *fsnimpl = fs->fsnimpl;
#ifdef  FSCACHE
	int		cached;
	nameinfo_t	nameinfo;
#endif

	assert(fsnimpl);
	assert(fs->croot);
	assert(fs->cwd);

#if VERBOSITY > 2
	printf(__FUNCTION__": looking up `%s'\n", name);
#endif

	/* Dont try looking up "" */
	if (!*name)
		return OSKIT_ENOENT;

	/* Look for "." */
	if (name[0] == '.' && name [1] == '\0') {
		dir = fs->cwd;
		if (wantparent) {
			rc = oskit_dir_lookup(dir, "..", &file);
			assert(!rc);
			dir = (oskit_dir_t *) file;
		}
		else {
			oskit_dir_addref(dir);
		}
		*isdir   = 1;
		*fileret = (oskit_file_t *) dir;
		return 0;
	}
	
	/* Copy the pathname so we can modify it */
	if (strlen(name) + 1 > sizeof(wname))
		return OSKIT_ENAMETOOLONG;
	strcpy(wname, name);
	s = wname;

	/* 
	 * On loop entry, we hold an additional ref to the base directory.
	 * Fail if the root or cwd haven't been initialized.
	 */
	dir = (wname[0] == '/') ? fs->croot : fs->cwd;
	oskit_dir_addref(dir);

	/*
	 * Note that, to be consistent with standard practice,
	 * a lookup of "foo/" succeeds only if 'foo' is a directory.
	 */
	while (1) {
		/* skip slashes */
		while (*s == '/')
			s++;

		if (!*s) {	/* this was the last component */
			if (wantparent) {
				rc = oskit_dir_lookup(dir, "..", &file);
				assert(!rc);
				oskit_dir_release(dir);
				dir = (oskit_dir_t *) file;
			}
			*isdir   = 1;
			*fileret = (oskit_file_t*)dir;
#if VERBOSITY > 2
			printf(__FUNCTION__" = success.\n");
#endif
			return 0;
		}

		/* Break out the next component name */
		comp = s;
		while (*s && *s != '/')
			s++;

		/* See if we're finding the parent of a mounted file system */
		if (comp[0] == '.' && comp[1] == '.' && (s == comp + 2)) {
			oskit_dir_query(dir, &oskit_iunknown_iid, (void**)&iu);
			FSLOCK(fsnimpl);
			mountparent:
			for (mp = fsnimpl->mountpoints; mp; mp = mp->next) {
				if (iu == mp->subtree_iu) {
					oskit_dir_release(dir);
					oskit_iunknown_release(iu);
					dir = mp->mountover;
					oskit_dir_addref(dir);
					iu = mp->mountover_iu;
					oskit_iunknown_addref(iu);
					goto mountparent;
				}
			}
			FSUNLOCK(fsnimpl);
			oskit_iunknown_release(iu);
		}

#if VERBOSITY > 2
		printf(__FUNCTION__": looking up in dir=%p, comp=`%s'\n",
		       dir, comp);
#endif
#ifdef  FSCACHE
		FSCACHE_LOCK(fsnimpl);
		if (!(cached =
		      fsn_cache_lookup(fsnimpl, dir, comp, s-comp, &nameinfo)))
#endif
		{
			savechar = *s;
			*s = 0;
			rc = oskit_dir_lookup(dir, comp, &file);
			*s = savechar;
			if (rc) {
				oskit_dir_release(dir);
				FSCACHE_UNLOCK(fsnimpl);
#if VERBOSITY > 2
				printf(__FUNCTION__" = nope: rc=%d, lookup failed.\n", rc);
#endif
				return rc;
			}
		}
#ifdef  FSCACHE
		/*
		 * Add the new entry to the cache. As a side effect, the
		 * cache enter code is going to determine a few things for
		 * us, and save that info away for next time. This will avoid
		 * having to relearn if the entry is a mountpoint, or a
		 * symlink, and whether its a file or directory.
		 */
		if (!cached)
			fsn_cache_enter(fsnimpl,
					dir, file, comp, s - comp, &nameinfo);
		else
			file = nameinfo.file;

#if VERBOSITY > 2
		printf(__FUNCTION__" -> got file %p (cached = %d; ni.flags = %d)\n",
		       file, cached, nameinfo.flags);
#endif

		FSCACHE_UNLOCK(fsnimpl);

		/*
		 * Skip the mountpoint code if we can
		 */
		if (! (nameinfo.flags & NAMEINFO_MOUNTPOINT))
			goto skipmount;
#endif

		/*
		 * See if it's a mount point and translate it if so
		 */
		oskit_file_query(file, &oskit_iunknown_iid, (void**)&iu);
		FSLOCK(fsnimpl);
     mountpoint:
		for (mp = fsnimpl->mountpoints; mp; mp = mp->next) {
			if (iu == mp->mountover_iu) {
				oskit_file_release(file);
				oskit_iunknown_release(iu);
				file = mp->subtree;
				oskit_file_addref(file);
				iu = mp->subtree_iu;
				oskit_iunknown_addref(iu);
				goto mountpoint;/* support multilevel mounts */
			}
		}
		FSUNLOCK(fsnimpl);
		oskit_iunknown_release(iu);
      skipmount:
		
		/*
		 * If last component and NOFOLLOW, then skip the symlink check.
		 */
		if (!*s && (flags & FSLOOKUP_NOFOLLOW))
			goto lastcomp;
#ifdef  FSCACHE
		/*
		 * Skip the symlink code if we can. By definition, if the dir
		 * was a mountpoint, it cannot be a symlink, so the fact that
		 * above code changed the actual file in question is okay.
		 */
		if (! (nameinfo.flags & NAMEINFO_SYMLINK))
			goto skiplink;
#endif
		/*
		 * See if it's a symlink, and translate it if so.
		 */
		rc = oskit_file_readlink(file, link, sizeof(link), &linksize);
		if (!rc) {
			oskit_file_release(file);
			
			/* Put together the new path to traverse */
			if (linksize + strlen(s) + 1 > sizeof(link)) {
				oskit_dir_release(dir);
#if VERBOSITY > 2
				printf(__FUNCTION__" = nope: ENAMETOOLONG.\n");
#endif
				return OSKIT_ENAMETOOLONG;
			}
			strcpy(link + linksize, s);
			strcpy(wname, link);
			s = wname;

			/* Avoid infinite symlink loops */
			if (symlinkdepth > MAXSYMLINKDEPTH) {
				/* too many nested symlinks */
				oskit_dir_release(dir);
#if VERBOSITY > 2
				printf(__FUNCTION__" = nope: ELOOP.\n");
#endif
				return OSKIT_ELOOP;
			}
			symlinkdepth++;

			/*
			 * If it's an absolute symlink,
			 * restart traversal at the root directory.
			 */
			if (wname[0] == '/') {
				oskit_dir_release(dir);
				dir = fs->croot;
				oskit_dir_addref(dir);
			}

			continue;
		}
       skiplink:
		/*
		 * Check for last component.
		 */
		if (!*s) {
		lastcomp:
#ifdef  FSCACHE
			/*
			 * Remove from cache if requested.
			 */
			if (flags & FSLOOKUP_NOCACHE)
				fsn_cache_delete(fsnimpl, dir, comp, s - comp);
			*isdir = nameinfo.flags & NAMEINFO_DIRECTORY;
#endif
			if (wantparent) {
				*fileret = (oskit_file_t *) dir;
				oskit_file_release(file);
			}
			else {
				*fileret = file;
				oskit_dir_release(dir);
			}
#if VERBOSITY > 2
			printf(__FUNCTION__" = success\n");
#endif
			return 0;
		}
		oskit_dir_release(dir);
#ifdef FSCACHE
		/*
		 * Skip the dir check code if we can.
		 */
		if (nameinfo.flags & NAMEINFO_DIRECTORY) {
			dir = (oskit_dir_t *) file;
			continue;
		}
		else {
#if VERBOSITY > 2
			printf(__FUNCTION__" = ENOTDIR (flags=%#x)\n",
			       nameinfo.flags);
#endif
			return OSKIT_ENOTDIR;
		}
#endif
		/* 
		 * see if it's a directory
		 */
		rc = oskit_file_query(file, &oskit_dir_iid, (void **)&dir);
		oskit_file_release(file);
#if VERBOSITY > 2
		printf(__FUNCTION__" = NOINTERFACE or NOTDIR, rc=%d\n", rc);
#endif
		if (rc)
			return (rc == OSKIT_E_NOINTERFACE) ? OSKIT_ENOTDIR : rc;
	}
}

/*
 * Lookup directory a file is in. Helper function that looks up the
 * file, but returns a pointer to the directory instead. Return a
 * pointer to the last component of the name to the caller.
 */
oskit_error_t
fsn_lookup_dir(struct fsobj *fs, const char *name,
	       int flags, char **comp, oskit_dir_t **dirret)
{
	oskit_file_t	*f = 0;
	oskit_error_t	rc;
	int		isdir;

	rc = fsn_lookup(fs, name,
			flags|FSLOOKUP_NOFOLLOW|FSLOOKUP_PARENT, &isdir, &f);
	if (!rc) {
		char	*n = strrchr(name, '/');

		if (n)
			*comp = n;
		*dirret = (oskit_dir_t *) f;
	}
	return rc;
}
