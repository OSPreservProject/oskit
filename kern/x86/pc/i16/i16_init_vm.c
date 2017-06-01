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

#include <oskit/x86/i16.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/base_vm.h>

CODE16

/*
 * This function is called by i16_crt0 (or the equivalent)
 * to set up our basic virtual memory layout variables
 * to settings appropriate when running in 16-bit real mode,
 * before calling i16_main().
 */
void i16_init_vm(void)
{
	/* Find our 16-bit code/data/everything segment.  */
	real_cs = get_cs();

	/*
	 * Find out where the bottom of physical memory is.
	 * (We won't be able to directly use it for 32-bit accesses
	 * until we actually get into 32-bit mode.)
	 */
	phys_mem_va = -((unsigned)real_cs << 4);

	/*
	 * The base of linear memory is at the same place,
	 * at least until we turn paging on.
	 */
	linear_base_va = phys_mem_va;
}

