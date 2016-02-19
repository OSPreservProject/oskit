/*
 * Copyright (c) 1994-1996, 1999 Sleepless Software
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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

#include <oskit/types.h>
#include <oskit/debug.h>
#include <oskit/arm32/trap.h>
#include <oskit/arm32/base_vm.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/proc_reg.h>
#include <oskit/arm32/gdb.h>

int
gdb_copyin(oskit_addr_t src_va, void *dest_buf, oskit_size_t size)
{
	assert(! interrupts_enabled());

	return gdb_copy((void *) src_va, dest_buf, size);
}

int
gdb_copyout(const void *src_buf, oskit_addr_t dest_va, oskit_size_t size)
{
	int		rc;

	assert(! interrupts_enabled());

	if ((rc = gdb_copy((void *) src_buf, (void *) dest_va, size)) != 0)
		return rc;

	/*
	 * Clean and Flush the address range just written.
	 */
	flush_cache_range_D(dest_va, size);
	flush_cache_I();

	return rc;
}
