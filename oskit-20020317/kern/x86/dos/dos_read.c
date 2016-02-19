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

int dos_read(dos_fd_t fd, void *buf, oskit_size_t size, oskit_size_t *out_actual)
{
	int err;
	int actual = 0;
	struct trap_state ts;
	oskit_addr_t dos_buf_phys = (oskit_addr_t)kvtophys(dos_buf);

	assert(dos_buf); assert(dos_buf_phys);
	assert(dos_buf_phys < 0x100000);

	init_ts(&ts);

	while (size > 0)
	{
		int little_size = size;
		int little_actual;

		if (little_size > DOS_BUF_SIZE)
			little_size = DOS_BUF_SIZE;

		ts.trapno = 0x21;
		ts.eax = 0x3f00;
		ts.ebx = fd;
		ts.ecx = little_size;
		ts.v86_ds = dos_buf_phys >> 4;
		ts.edx = dos_buf_phys & 15;
		base_real_int(&ts);
		if ((err = dos_check_err(&ts)) != 0)
			return err;
		little_actual = ts.eax & 0xffff;
		assert(little_actual <= little_size);

		/* XXX don't copy if buf is <1MB */
		memcpy(buf, dos_buf, little_actual);

		buf += little_actual;
		size -= little_actual;
		actual += little_actual;

		if (little_actual < little_size)
			break;
	}

	*out_actual = actual;
	return 0;
}
