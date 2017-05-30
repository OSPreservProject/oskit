/*
 * Copyright (c) 1997-1998, 2000 University of Utah and the Flux Group.
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

#include <unistd.h>
#include <sys/types.h>
#include "fd.h"

int
ftruncate(int fd, off_t length)
{
	oskit_file_t	*f;
	oskit_error_t	rc;
	oskit_stat_t	st;

	if (fd_check_openfile(fd))
		return -1;
	fd_access_lock(fd, FD_RDWR);

	/* Find the underlying file referred to by this openfile */
	rc = oskit_openfile_getfile(fd_array[fd].openfile, &f);
	if (rc) {
		fd_unlock(fd);
		errno = rc;
		return -1;
	}

	st.size = length;
	rc = oskit_file_setstat(f, OSKIT_STAT_SIZE, &st);
	oskit_file_release(f);
	fd_access_unlock(fd, FD_RDWR);

        return rc ? (errno = rc, -1) : 0;
}

