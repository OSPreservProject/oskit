/*
 * Copyright (c) 1994-1995, 1998, 1999 University of Utah and the Flux Group.
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
#include <errno.h>
#include <strings.h>
#include <termios.h>

#include "dos_io.h"

int dos_tcgetattr(dos_fd_t fd, struct termios *t)
{
	int err;
	struct trap_state ts;

	init_ts(&ts);

	bzero(t, sizeof(*t));

	/* First make sure this is actually a character device.  */
	ts.trapno = 0x21;
	ts.eax = 0x4400;
	ts.ebx = fd;
	base_real_int(&ts);
	if ((err = dos_check_err(&ts)) != 0)
		return err;
	if (!(ts.edx & (1<<7)))
		return ENOTTY;

	return 0;
}
