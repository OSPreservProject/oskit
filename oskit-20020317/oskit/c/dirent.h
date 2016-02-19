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

#ifndef _OSKIT_C_DIRENT_H_
#define _OSKIT_C_DIRENT_H_

#include <oskit/compiler.h>
#include <oskit/fs/dir.h>

/*
 * structure describing a directory entry
 */
struct dirent {
	oskit_ino_t	d_ino;		/* X/Open CAE d_ino entry */
	char		d_name[1];	/* POSIX 1003.1 d_name entry */
};

/*
 * structure describing an open directory
 */
typedef struct _dir {
	oskit_dir_t		*dir;
	oskit_dirents_t 	*dirents;
	oskit_u32_t		deofs;
	oskit_u32_t		offset;
	oskit_s32_t		count;
	struct dirent		*dent;
} DIR;

OSKIT_BEGIN_DECLS

/* POSIX 1003.1 functions */
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dir);
void rewinddir(DIR *dir);
int closedir(DIR *dir);

OSKIT_END_DECLS

#endif /* _OSKIT_C_DIRENT_H_ */
