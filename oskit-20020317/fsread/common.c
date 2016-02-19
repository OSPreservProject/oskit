/*
 * Copyright (c) 1998 University of Utah and the Flux Group.
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
#include <oskit/io/blkio.h>
#include "common.h"
#include <oskit/c/string.h>

struct file {
	oskit_blkio_t	bio;
	unsigned	count;
};

/*
 * Query for available interfaces
 */
OSKIT_COMDECL
fsread_common_query(oskit_blkio_t *io,
		    const struct oskit_guid *iid,
		    void **out_ihandle)
{
	struct file *fp = (struct file *)io;

	if (memcmp(iid, &oskit_blkio_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &fp->bio;
		fp->count++;
		return 0;
	}
	return OSKIT_E_NOINTERFACE;
}

/*
 * Add a reference
 */
OSKIT_COMDECL_U
fsread_common_addref(oskit_blkio_t *io)
{
	struct file *fp = (struct file *)io;

	return ++fp->count;
}

/*
 * Return the block size for this file,
 * which is always 1 because we support arbitrarily-aligned reads.
 */
OSKIT_COMDECL_U
fsread_common_getblocksize(oskit_blkio_t *io)
{
	return 1;
}

OSKIT_COMDECL
fsread_common_write(oskit_blkio_t *io, const void *start, oskit_off_t offset,
		    oskit_size_t size, oskit_size_t *out_actual)
{
	return OSKIT_E_NOTIMPL;
}

OSKIT_COMDECL
fsread_common_setsize(oskit_blkio_t *io, oskit_off_t size)
{
	return OSKIT_E_NOTIMPL;
}
