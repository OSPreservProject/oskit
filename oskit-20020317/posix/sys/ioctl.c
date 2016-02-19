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
#include <stdarg.h>
#include <sys/filio.h>
#include <termios.h>
#include <assert.h>
#include <sys/ttycom.h>
#include "fd.h"

#include <oskit/io/asyncio.h>
#include <oskit/io/ttystream.h>

static int tty_ioctl(int fd, unsigned long request, char *argp);

int     
ioctl(int fd, unsigned long request, ...)
{
	oskit_error_t	rc;
	int		ret = -1;
	char		*argp;
        va_list vl;

	va_start(vl, request);
	argp = va_arg(vl, char *);
	va_end(vl);

	/*
	 * Look at the ioctl group, and dispatch. More to come ...
	 */
	if (IOCGROUP(request) == 't')
		return tty_ioctl(fd, request, argp);

	if (fd_check(fd))
		return -1;
	fd_access_lock(fd, FD_RDWR);

	/*
	 * XXX: do some systematic processing here, IOR/IOW etc.
	 */

	/* hack up FIONREAD */
	if (request == FIONREAD) {
		int *ip = (int *)argp;
		if (!FD_HAS_INTERFACE(fd, asyncio)) {
			rc  = ENOTTY;
			ret = -1;
			goto bad;
		}

		rc = oskit_asyncio_readable(fd_array[fd].asyncio);
		if (rc < 0)
			goto bad;

		*ip = rc;
		ret = 0;
		goto good;
	} 
	rc = EINVAL;

bad:
	errno = rc;
good:
	fd_access_unlock(fd, FD_RDWR);
	return ret;
}

static int
tty_ioctl(int fd, unsigned long request, char *argp)
{
	oskit_termios_t		*t = (oskit_termios_t *) argp;
	oskit_ttystream_t	*ttystream;
	oskit_error_t		rc;

	if (fd_check_ttystream(fd))
		return -1;
	fd_access_lock(fd, FD_RDWR);

	ttystream = fd_array[fd].ttystream;
	assert(ttystream);

	switch(request) {
	case TIOCGETA:
		rc = oskit_ttystream_getattr(ttystream, t);
		break;

	case TIOCSETA:
		rc = oskit_ttystream_setattr(ttystream, OSKIT_TCSANOW, t);
		break;
		
	case TIOCSETAW:
		rc = oskit_ttystream_setattr(ttystream, OSKIT_TCSADRAIN, t);
		break;
		
	case TIOCSETAF:
		rc = oskit_ttystream_setattr(ttystream, OSKIT_TCSAFLUSH, t);
		break;

	case TIOCDRAIN:
		rc = oskit_ttystream_drain(ttystream);
		break;

	case TIOCSTOP:
		rc = oskit_ttystream_flow(ttystream, OSKIT_TCOOFF);
		break;

	case TIOCSTART:
		rc = oskit_ttystream_flow(ttystream, OSKIT_TCOON);
		break;

	case TIOCFLUSH:
		rc = oskit_ttystream_flush(ttystream, *((int *) argp));
		break;
		
	case TIOCNOTTY:
		rc = 0;
		break;

	default:
		rc = EINVAL;
		break;
	}
	fd_access_unlock(fd, FD_RDWR);

	return rc ? (errno = rc), -1 : 0;
}
