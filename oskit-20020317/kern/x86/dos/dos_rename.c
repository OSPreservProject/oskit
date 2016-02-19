/*
 * Copyright (c) 1994-1998 University of Utah and the Flux Group.
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
#include <oskit/x86/base_vm.h>
#include <oskit/c/string.h>

#include "dos_io.h"

int dos_rename(const char *oldpath, const char *newpath)
{
	/* This function isn't fully atomic like on Unix,
	   but it's as close as I know how to do under DOS.  */

	struct trap_state ts;
	oskit_addr_t dos_buf_phys = (oskit_addr_t)kvtophys(dos_buf);
	int err;

	init_ts(&ts);

	/* Put the oldpath in the first half of dos_buf,
	   and the newpath in the second half.  */
	if ((strlen(oldpath)+1 > DOS_BUF_SIZE/2)
	    || (strlen(newpath)+1 > DOS_BUF_SIZE/2))
		{ errno = E2BIG; return -1; }
	strcpy(dos_buf, oldpath);
	strcpy(dos_buf+DOS_BUF_SIZE/2, newpath);

	/* Try once to rename the file.  */
	ts.trapno = 0x21;
	ts.eax = 0x5600;
	ts.v86_ds = dos_buf_phys >> 4;
	ts.edx = dos_buf_phys & 15;
	ts.v86_es = dos_buf_phys >> 4;
	ts.edi = (dos_buf_phys & 15) + DOS_BUF_SIZE/2;
	base_real_int(&ts);

	/* If that failed, delete newpath and then retry the rename.
	   We _hope_ the failure was because newpath already existed;
	   the DOS error codes I'm getting back seem to be bogus.  */
	if ((err = dos_check_err(&ts)) != 0)
	{
		ts.trapno = 0x21;
		ts.eax = 0x4100;
		ts.v86_ds = dos_buf_phys >> 4;
		ts.edx = (dos_buf_phys & 15) + DOS_BUF_SIZE/2;
		base_real_int(&ts);
		if ((err = dos_check_err(&ts)) != 0)
			return err;

		ts.trapno = 0x21;
		ts.eax = 0x5600;
		ts.v86_ds = dos_buf_phys >> 4;
		ts.edx = dos_buf_phys & 15;
		ts.v86_es = dos_buf_phys >> 4;
		ts.edi = (dos_buf_phys & 15) + DOS_BUF_SIZE/2;
		base_real_int(&ts);
		err = dos_check_err(&ts);
	}

	return err;
}
