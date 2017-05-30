/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the OSKit Filesystem Reading Library, which is free
 * software, also known as "open source;" you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (GPL), version 2, as
 * published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Wrapper for FS format-specific reader modules
 * to recognize a file system automatically and open a file on it.
 */

#include <oskit/fs/read.h>

oskit_error_t
fsread_open(oskit_blkio_t *bio, const char *path, oskit_blkio_t **out_fbio)
{
	oskit_error_t rc;

	rc = fsread_ffs_open(bio, path, out_fbio);
	if (rc == 0)
		return 0;

	rc = fsread_ext2_open(bio, path, out_fbio);
	if (rc == 0)
		return 0;

	rc = fsread_minix_open(bio, path, out_fbio);
	if (rc == 0)
		return 0;

	return rc;
}

