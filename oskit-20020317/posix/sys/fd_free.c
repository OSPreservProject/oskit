/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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

#include "fd.h"

#include <oskit/com/stream.h>
#include <oskit/fs/openfile.h>
#include <oskit/net/socket.h>
#include <oskit/io/ttystream.h>
#include <oskit/io/asyncio.h>
#include <oskit/io/bufio.h>

#define RELEASE(fd, intf, type)						\
	if (fd_array[fd].intf) {					\
		type##_release(fd_array[fd].intf);			\
		fd_array[fd].intf = 0;					\
	}

/*
 * Assumption: 
 * fd_layer is already locked, will stay locked, and fd is already checked
 */
int
fd_free(int fd)
{
	fd_array_lock();
	
	/* 
	 * this is the only check we perform, for the convenience 
	 * of callers like dup2 and fd_set_console 
	 */
	if (fd >= fd_arraylen) {
		fd_array_unlock();
		return EBADF;
	}

	/* Release the primary object reference */
	/* this marks the slot as unused, too */
	RELEASE(fd, obj, oskit_iunknown);

	/* Release any other cached interface pointers */
	RELEASE(fd, stream, oskit_stream);
	RELEASE(fd, posixio, oskit_posixio);
	RELEASE(fd, openfile, oskit_openfile);
	RELEASE(fd, socket, oskit_socket);
	RELEASE(fd, ttystream, oskit_ttystream);

	/* Be nice and remove the listener from the asyncio interface */
	if (fd_array[fd].listener)
		oskit_asyncio_remove_listener( fd_array[fd].asyncio, 
					      fd_array[fd].listener);
	RELEASE(fd, listener, oskit_listener);
	RELEASE(fd, asyncio, oskit_asyncio);
	RELEASE(fd, bufio, oskit_bufio);

#ifdef  THREAD_SAFE
#ifdef  DEBUG
	fd_array[fd].creator = (pthread_t) -1;
#endif
#endif
	fd_array_unlock();
	return 0;
}

