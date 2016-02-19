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

#include <unistd.h>
#include <oskit/dev/dev.h>
#include <oskit/c/environment.h>
#include <oskit/dev/osenv_intr.h>
#include "fd.h"
#include "posix.h"

int
write(int fd, const void *buf, size_t size)
{
	oskit_u32_t	retval;
	oskit_error_t	rc;
#ifdef	THREAD_SAFE
	int		enabled = 0;

	/*
	 * Sigh. Don't have a good solution for protecting against
	 * printf out of interrupt handlers.
	 */
	if (fd == STDOUT_FILENO) {
#ifdef KNIT
// CpU achieves this effect by wrapping printf in protective code.
//              enabled = osenv_intr_save_disable();
#else
		if (!posixlib_osenv_intr_iface)
			posixlib_osenv_intr_init_iface();
		if (posixlib_osenv_intr_iface)
			enabled = oskit_osenv_intr_save_disable(
						posixlib_osenv_intr_iface);
#endif /* !KNIT */
	}
	else
		pthread_testcancel();
#endif
	if (fd_check_stream(fd)) {
		rc = errno;
        } else {
                fd_access_lock(fd, FD_WRITE);
                rc = oskit_stream_write(fd_array[fd].stream, buf, size, 
                                        &retval);
                fd_access_unlock(fd, FD_WRITE);
        }

#ifdef  THREAD_SAFE
	if (fd == STDOUT_FILENO) {
#ifdef KNIT                
//              if (enabled)
//                      osenv_intr_enable();
#else
		if (posixlib_osenv_intr_iface && enabled)
			oskit_osenv_intr_enable(posixlib_osenv_intr_iface);
#endif /* !KNIT */
	}
#endif
	if (rc) {
		errno = rc;
		return -1;
	}
	return retval;
}

