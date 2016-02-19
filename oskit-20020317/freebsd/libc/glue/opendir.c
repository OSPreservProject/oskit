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
 * This is dumb. Why is the oskit COM interface for getdirentries defined
 * so that it is such a mismatch for its intended use?
 */

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include <oskit/dev/dev.h>
#include <oskit/c/fs.h>
#include <oskit/c/environment.h>
#include <oskit/dev/osenv_mem.h>

#ifndef VERBOSITY
#define VERBOSITY 0
#endif

DIR *
opendir(const char *name)
{
	oskit_file_t	*f;
	oskit_dir_t	*d;
	DIR	 	*dd;

	oskit_error_t rc = fs_lookup(name, FSLOOKUP_FOLLOW, &f);
	if (rc)
		return errno = rc, (DIR *)0;

	rc = oskit_file_query(f, &oskit_dir_iid, (void**)&d);
	oskit_file_release(f);
	if (rc) {
		if (rc == OSKIT_E_NOINTERFACE)
			rc = ENOTDIR;
		return errno = rc, (DIR *)0;
	}

	dd = malloc(sizeof *dd);
	if (dd == NULL) {
		oskit_dir_release(d);
		return errno = ENOMEM, (DIR *)0;
	}
	memset(dd, 0, sizeof *dd);
	dd->dir = d;
	return dd;
}

struct dirent *
readdir(DIR *dd)
{
	oskit_error_t rc;
	char buf[256];
	struct oskit_dirent *fd = (struct oskit_dirent *) buf;
	struct dirent *dir;

	if (dd->dent) {
		free(dd->dent);
		dd->dent = 0;
	}

 tryagain:
	/* do we need to read more? */
	if (!dd->dirents || dd->deofs >= dd->count) {
		if (dd->dirents)
			oskit_dirents_release(dd->dirents);
		rc = oskit_dir_getdirentries(dd->dir, &dd->offset, 
			1, &dd->dirents);
		if (rc) 
			return errno = rc, (struct dirent *)0;
		
		dd->deofs = 0;
		dd->count = 0;
		if (dd->dirents)
			oskit_dirents_getcount(dd->dirents, &dd->count);
	}

	/* we tried to read, but ran out -> we're done */
	if (!dd->count) 
		return (struct dirent *)0;

	/* Request the next dirent from the COM object */
	fd->namelen = (256 - sizeof(*fd));
	if ((rc = oskit_dirents_getnext(dd->dirents, fd)))
		return errno = rc, (struct dirent *)0;
	dd->deofs++;	

	/* skip zero-inode files */
	if (fd->ino == 0)
		goto tryagain;
	
	dd->dent = dir = malloc(sizeof(struct dirent));
	if (dir == 0)
		return errno = ENOMEM, (struct dirent *)0;
	dir->d_fileno = fd->ino;
	dir->d_reclen = sizeof(dir);
	dir->d_type   = DT_UNKNOWN;
	dir->d_namlen = fd->namelen;
	strcpy(dir->d_name, fd->name);
	return dir;
}

void 
rewinddir(DIR *dd)
{
	/* let's start over */
	if (dd->dirents) {
		oskit_dirents_release(dd->dirents);
		dd->dirents = 0;
	}
	dd->offset = 0;
}

int 
closedir(DIR *dd)
{
	if (dd->dent)
		free(dd->dent);
	if (dd->dirents)
		oskit_dirents_release(dd->dirents);
	oskit_dir_release(dd->dir);
	free(dd);
	return 0;
}

