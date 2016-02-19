/*
 * Copyright (c) 2001 University of Utah and the Flux Group.
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
 * Get absio/blkio COM object from a file descriptor
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <oskit/io/absio.h>
#include "fd.h"

extern oskit_error_t
fd_get_absio(int fd, oskit_absio_t **out_absio)
{
    oskit_openfile_t *openfile;
    oskit_file_t *file;
    oskit_absio_t *absio;

    if (fd_check(fd))
	return -1;

    openfile = fd_array[fd].openfile;
    fd_unlock(fd);
    if (oskit_openfile_getfile(openfile, (oskit_file_t**)&file)) {
	return OSKIT_ENOTSUP;
    }
    oskit_file_query(file, &oskit_absio_iid, (void**)&absio);
    oskit_file_release(file);

    if (absio == 0) {
	return OSKIT_ENOTSUP;
    }

    *out_absio = absio;

    return 0;
}
