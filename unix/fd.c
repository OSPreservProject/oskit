/*
 * Copyright (c) 1994-2001 University of Utah and the Flux Group.
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
 * This file exists to map the threaded calls to regular system calls
 * in single-threaded applications.  For multithreaded applications,
 * threaded_fd contains the right versions.
 *
 * These functions set NATIVEOS(errno) if there is a problem.
 * The *do not* remap the error onto errno.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "native.h"
#include "support.h"

/*
 * Blocking read from an fd.
 */
int
oskitunix_threaded_read(int fd, void* buf, size_t len)
{
	return NATIVEOS(read)(fd, buf, len);
}

/*
 * Blocking write to an fd.
 */
int
oskitunix_threaded_write(int fd, const void* buf, size_t len)
{
	return NATIVEOS(write)(fd, buf, len);
}

/*
 * Blocking socket connect.
 */
int
oskitunix_threaded_connect(int fd, struct sockaddr* addr, size_t len)
{
	return NATIVEOS(connect)(fd, addr, len);
}

/*
 * Blocking socket accept.
 */
int
oskitunix_threaded_accept(int fd, struct sockaddr* addr, size_t* len)
{
	return NATIVEOS(accept)(fd, addr, len);
}

/*
 * Blocking socket recvfrom 
 */
int
oskitunix_threaded_recvfrom(int fd, void* buf, size_t len, int flags, 
	struct sockaddr* from, int* fromlen)
{
	return NATIVEOS(recvfrom)(fd, buf, len, flags, from, fromlen);
}

/*
 * Blocking socket sendto
 */
int
oskitunix_threaded_sendto(int fd, const void* buf, size_t len, int flags, 
	struct sockaddr* to, int tolen)
{
	return NATIVEOS(sendto)(fd, buf, len, flags, to, tolen);
}

/*
 * In single threaded kernels, these are a no-ops.
 * The multithread versions are in threaded_fd.c
 */

int
oskitunix_set_async_fd(int fd)
{
	return 0;
}

int
oskitunix_unset_async_fd(int fd)
{
	return 0;
}
