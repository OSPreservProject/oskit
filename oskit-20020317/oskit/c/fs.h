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
 * This header file defines common types and functions used by
 * the minimal C library's default POSIX file system functions
 * (open, rename, link, unlink, symlink, etc.).
 * This header file is NOT a standard POSIX or Unix header file;
 * instead its purpose is to expose the implementation of this facility
 * so that the client can fully control it and use it in arbitrary contexts.
 *
 * The main function of this module is to keep track of
 * a root directory and a current working directory.
 * Upon initialization with a oskit_dir_t posing as rootdir,
 * it will perform open, access etc. with pathnames relative to the root.
 */
#ifndef _OSKIT_C_FS_H_
#define _OSKIT_C_FS_H_

#include <oskit/c/errno.h>
#include <oskit/fs/netbsd.h>
#include <oskit/fs/filesystem.h>
#include <oskit/fs/file.h>
#include <oskit/fs/openfile.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/fsnamespace.h>
#include <oskit/compiler.h>

#ifdef KNIT
extern oskit_mode_t	fs_cmask; /* Creation mask on permissions (umask).  */
#else
oskit_mode_t		fs_cmask; /* Creation mask on permissions (umask).  */
#endif

OSKIT_BEGIN_DECLS

/*
 * BSD-like mount/unmount functions which the client can use
 * to build its file system namespace out of multiple file systems.
 * The fs_lookup() function below takes care of crossing mount points
 * while traversing the file system namespace.
 */
oskit_error_t fs_mount(const char *path, oskit_file_t *subtree);
oskit_error_t fs_unmount(const char *path);


/*** Internal functions and macros ***/
/*
 * The client normally doesn't need to use these directly,
 * but it can if it wants to do other special things.
 */

/* just do lookup - needed for opendir */
oskit_error_t fs_lookup(const char *name, int flags, oskit_file_t **out_file);

/* look up the dir a file is in */
oskit_error_t fs_lookup_dir(const char **inout_name, int flags,
			    oskit_dir_t **out_dir);

#define FSLOOKUP_NOFLAGS	0x00	/* Normal operation */
#define FSLOOKUP_FOLLOW		0x00	/* Normal operation */
#define FSLOOKUP_NOFOLLOW	0x01	/* Do not follow last symlink */
#define FSLOOKUP_NOCACHE	0x02	/* Do not leave entry in the cache */
#define FSLOOKUP_PARENT		0x04	/* Return parent instead */

OSKIT_END_DECLS

#endif /* _OSKIT_C_FS_H_ */
