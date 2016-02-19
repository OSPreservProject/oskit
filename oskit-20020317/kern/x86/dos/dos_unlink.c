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
#include <oskit/x86/base_vm.h>
#include <oskit/c/string.h>

#include "dos_io.h"

int dos_unlink(const char *filename)
{
	struct trap_state ts;
	oskit_addr_t dos_buf_phys = (oskit_addr_t)kvtophys(dos_buf);

	init_ts(&ts);

	if (strlen(filename)+1 > DOS_BUF_SIZE)
		return E2BIG;
	strcpy(dos_buf, filename);

	ts.trapno = 0x21;
	ts.eax = 0x4100;
	ts.v86_ds = dos_buf_phys >> 4;
	ts.edx = dos_buf_phys & 15;
	base_real_int(&ts);

	return dos_check_err(&ts);
}
