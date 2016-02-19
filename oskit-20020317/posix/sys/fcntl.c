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

/*
 * Stole this from Fluke. Hope thats okay!
 */

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include "fd.h"

int
fcntl(int fd, int cmd, ...)
{
	va_list		ap;
	fd_t		*fdp;
	int		result;

	if (fd_check(fd))
		return -1;
	fdp = fd_array + fd;

	va_start (ap, cmd);

	switch (cmd) {
	default:			/* Bad command.  */
		errno = EINVAL;
		result = -1;
		break;


	case F_GETFD:		/* Get descriptor flags.  */
		result = fdp->flags;
		break;

	case F_SETFD:		/* Set descriptor flags.  */
		result = va_arg (ap, int);
		if (result &~ FD_CLOEXEC)
		{
			errno = EINVAL;
			result = -1;
		}
		else
		{
			fdp->flags = result;
			result = 0;
		}
		break;

	case F_DUPFD:		/* Duplicate the file descriptor.  */
	{
		/*
		 * Third argument is the minimum file descriptor number to
		 * return.
		 */
		int newfd = fd_alloc(fdp->obj, va_arg(ap, int));

		if (newfd)
			result = newfd;
		else
			result = -1;
		break;
	}

	case F_GETFL:		/* Get per-open flags.  */

		/* Return this so fdopen doesn't complain.  */
		result = O_ACCMODE;
		break;

	case F_SETFL:		/* Set per-open flags.  */

	case F_GETOWN:		/* Get owner.  */

	case F_SETOWN:		/* Set owner.  */

	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:

		errno = ENOSYS;
		result = -1;
		break;
	}

	va_end (ap);

	fd_unlock(fd);
	return result;
}
