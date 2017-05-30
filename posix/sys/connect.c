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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "fd.h"

int
connect(int fd, const struct sockaddr *name, int namelen)
{
	oskit_error_t rc;

	if (fd_check_socket(fd))
		return -1;
	fd_access_lock(fd, FD_RDWR);

	rc = oskit_socket_connect(fd_array[fd].socket, 
		(struct oskit_sockaddr *)name, namelen);
	
	fd_access_unlock(fd, FD_RDWR);

	return rc ? (errno = rc, -1) : 0;
}

