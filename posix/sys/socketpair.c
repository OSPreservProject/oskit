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

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include "fd.h"
#include <oskit/c/environment.h>

#ifdef KNIT
extern oskit_socket_factory_t	*oskit_socket_factory;
#define fsc oskit_socket_factory
#else
static	oskit_socket_factory_t	*fsc;
#endif

/*
 * Create a pair of connected sockets.
 */
int
socketpair(int domain, int type, int protocol, int out_fd[2])
{
	oskit_error_t	rc;
	oskit_socket_t	*sock1, *sock2;
	int		fd1, fd2;

#ifndef KNIT
	if (fsc == 0) {
		/* note that lookup_first always returns null */
		oskit_library_services_lookup(&oskit_socket_factory_iid,
					      (void *) &fsc);
	}

	if (fsc == 0) {
		/*
		 * If we don't have a socket factory,
		 * we just don't support any protocols...
		 */
		errno = EPROTONOSUPPORT;
		return -1;
	}
#endif

	/* Create the socketpair */
	rc = oskit_socket_factory_create_pair(fsc, domain, type, protocol,
					      &sock1, &sock2);
	if (rc) {
		errno = rc;
		return -1;
	}

	/* Allocate a file descriptor for it */
	fd1 = fd_alloc((oskit_iunknown_t*)sock1, 0);
	if (fd1 < 0) {
		oskit_socket_release(sock1);
		oskit_socket_release(sock2);
		return -1;
	}
	fd2 = fd_alloc((oskit_iunknown_t*)sock2, 0);
	if (fd2 < 0) {
		fd_free(fd1);
		oskit_socket_release(sock1);
		oskit_socket_release(sock2);
		return -1;
	}
	fd_lock(fd1);		/* XXX want to lock for both together */
	fd_array[fd1].socket = sock1;
	fd_unlock(fd1);
	fd_lock(fd2);
	fd_array[fd2].socket = sock2;
	fd_unlock(fd2);

	out_fd[0] = fd1;
	out_fd[1] = fd2;
	return 0;
}
