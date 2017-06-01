/*
 * Copyright (c) 1994-1995, 1998 University of Utah and the Flux Group.
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
#include <string.h>
#include <sys/stat.h>

#include "dos_io.h"

int dos_fstat(dos_fd_t fd, struct stat *st)
{
	int err;
	struct trap_state ts;
	oskit_addr_t old_pos, new_pos;

	init_ts(&ts);

	memset(st, 0, sizeof(*st));
	st->st_nlink = 1;
	st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO; /* XXX attributes */

	/* Get device information,
	   which will tell us whether this is a character device
	   or a regular file.  */
	ts.trapno = 0x21;
	ts.eax = 0x4400;
	ts.ebx = fd;
	base_real_int(&ts);
	if ((err = dos_check_err(&ts)) != 0)
		return err;
	if (ts.edx & (1<<7))
		st->st_mode |= S_IFCHR;
	else
		st->st_mode |= S_IFREG;

	/* XXX get date/time with int 21 5700 */

	/* Get file size by seeking to the end and back.  */
	if (!dos_seek(fd, 0, 1, &old_pos)
	    && !dos_seek(fd, 0, 2, (oskit_addr_t *)&st->st_size))
	{
		if ((err = dos_seek(fd, old_pos, 0, &new_pos)) != 0)
			return err;
		if (new_pos != old_pos)
			return EIO;/*XXX*/
	}

	/* Always assume 512-byte blocks for now... */
	st->st_blocks = (st->st_size + 511) / 512;
	st->st_blksize = 512;

	return 0;
}

