/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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

#include <oskit/debug.h>
#include <oskit/c/stdio.h>
#include <oskit/x86/eflags.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/pc/phys_lmm.h>
#include <oskit/x86/pc/base_real.h>

void base_ext_mem_init(void)
{
	extern char _start[], _end[];
	struct trap_state ts;
	unsigned kern_start_pa = kvtophys(_start);
	unsigned kern_end_pa = kvtophys(_end);
	unsigned ext_start_pa = 0x100000;
	unsigned ext_end_pa;

	/* Ask the BIOS for the size of extended memory */
	ts.trapno = 0x15;
	ts.eax = 0x8800;
	ts.eflags = 0;
	base_real_int(&ts);
	if (ts.eflags & EFL_CF)
		return;
	ext_end_pa = 0x100000 + ((ts.eax & 0xffff) * 1024);
	if (ext_end_pa <= ext_start_pa)
		return;

	/* XXX check for bottom-up memory allocations */

	printf("Extended memory: %dK\n",
		(ext_end_pa - ext_start_pa) / 1024);

	/* Add the memory to our LMM pool */
#ifndef KNIT
	phys_lmm_init();
#endif
	if (kern_start_pa >= ext_end_pa || kern_end_pa <= ext_start_pa) {
		phys_lmm_add(ext_start_pa, ext_end_pa - ext_start_pa);
	} else {
		if (kern_start_pa > ext_start_pa)
			phys_lmm_add(ext_start_pa,
				     kern_start_pa - ext_start_pa);
		if (kern_end_pa < ext_end_pa)
			phys_lmm_add(kern_end_pa,
				     ext_end_pa - kern_end_pa);
	}
}
