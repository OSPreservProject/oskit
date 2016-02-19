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

#include <oskit/fs/file.h>
#include <oskit/fs/dir.h>
#include <stdlib.h>
#include <malloc.h>
#include "fsn.h"

/*
 * mount a file (dir or file) over a directory.
 */
oskit_error_t
fsn_mount(struct fsnimpl *fsnimpl, oskit_dir_t *dir, oskit_file_t *subtree)
{
	struct fs_mountpoint   *mp;

	/* Create a mountpoint node for it */
	mp = (struct fs_mountpoint *) smalloc(sizeof(*mp));
	if (mp == NULL)
		return OSKIT_ENOMEM;

	mp->subtree = subtree;
	oskit_file_addref(subtree);
	oskit_file_query(subtree, &oskit_iunknown_iid,(void**)&mp->subtree_iu);
	
	mp->mountover = dir;
	oskit_dir_query(dir, &oskit_iunknown_iid, (void**)&mp->mountover_iu);
	
	FSLOCK(fsnimpl);
	mp->next = fsnimpl->mountpoints;
	fsnimpl->mountpoints = mp;
	FSUNLOCK(fsnimpl);

	return 0;
}

oskit_error_t
fsn_unmount(struct fsnimpl *fsnimpl, oskit_file_t *file)
{
	oskit_iunknown_t	*iu;
	struct fs_mountpoint	**mpp, *mp;

	oskit_file_query(file, &oskit_iunknown_iid, (void**)&iu);
	oskit_file_release(file);

	FSLOCK(fsnimpl);
	/* Find its mountpoint node */
	for (mpp = &fsnimpl->mountpoints; ; mpp = &mp->next) {
		mp = *mpp;
		if (mp == NULL) {
			oskit_iunknown_release(iu);
			FSUNLOCK(fsnimpl);
			return OSKIT_EINVAL;
		}
		if (iu == mp->subtree_iu) {
			oskit_iunknown_release(iu);
			break;
		}
	}

	/* Remove and free the node */
	oskit_file_release(mp->subtree);
	oskit_iunknown_release(mp->subtree_iu);
	oskit_dir_release(mp->mountover);
	oskit_iunknown_release(mp->mountover_iu);
	*mpp = mp->next;
	FSUNLOCK(fsnimpl);
	sfree(mp, sizeof(*mp));

	return 0;
}

