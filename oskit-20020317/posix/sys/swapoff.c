/*
 * Copyright (c) 2001 The University of Utah and the Flux Group.
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

#include <oskit/c/stdio.h>
#include <oskit/c/unistd.h>
#include <oskit/c/sys/types.h>
#include <oskit/io/absio.h>
#include <oskit/c/fcntl.h>
#include <oskit/c/fd.h>
#include <oskit/uvm.h>

extern int
swapoff(const char *path)
{
    int fd;
    oskit_absio_t *absio;
    oskit_error_t error;

    fd = open(path, O_RDWR);
    if (fd == -1) {
	return -1;
    }

    error = fd_get_absio(fd, &absio);
    if (error) {
	errno = error;
	return -1;
    }

    error = oskit_uvm_swap_off((oskit_iunknown_t*)absio);
    oskit_absio_release(absio); /* don't need anymore */
    if (error) {
	errno = error;
	return -1;
    }
    return 0;
}
