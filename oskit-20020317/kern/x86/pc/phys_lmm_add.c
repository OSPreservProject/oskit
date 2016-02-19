/*
 * Copyright (c) 1995, 1998, 1999 University of Utah and the Flux Group.
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

#include <oskit/lmm.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/physmem.h>
#include <oskit/x86/pc/phys_lmm.h>

void phys_lmm_add(oskit_addr_t min_pa, oskit_size_t size)
{
	oskit_addr_t max_pa = min_pa + size;
	oskit_addr_t min_va = phystokv(min_pa);
	oskit_addr_t max_va = phystokv(max_pa);

	/*
	 * Add the memory region to the malloc_lmm.
	 * Because of virtual-to-physical translation,
	 * a physical region may wrap around virtual address zero.
	 * Naturally, the LMM doesn't like this,
	 * so we have to split it appropriately here.
	 * The LMM package will take care of splitting it up further
	 * if necessary according to the PC's physical memory regions.
	 */
	if (max_va >= min_va) {
		lmm_add_free(&malloc_lmm, (void*)min_va, max_va - min_va);
	} else {
		lmm_add_free(&malloc_lmm, (void*)min_va, 0xffffffff - min_va);
		lmm_add_free(&malloc_lmm, (void*)0, max_va);
	}

	/* Keep track of the maximum/minumum physical address seen */
	if (max_pa > phys_mem_max)
		phys_mem_max = max_pa;
	if (min_pa < phys_mem_min)
		phys_mem_min = min_pa;
}

