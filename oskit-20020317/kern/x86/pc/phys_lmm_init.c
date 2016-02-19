/*
 * Copyright (c) 1995-1999 University of Utah and the Flux Group.
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

static int inited;
static struct lmm_region reg1mb[2], reg16mb[2], reghigh[2];
lmm_t malloc_lmm = LMM_INITIALIZER;

static void add(struct lmm_region *reg,
		oskit_addr_t start_pa, oskit_addr_t end_pa,
		unsigned flags, unsigned pri)
{
	oskit_addr_t start_va = phystokv(start_pa);
	oskit_addr_t end_va = phystokv(end_pa);

	/*
	 * Because of virtual-to-physical translation,
	 * a physical region may wrap around virtual address zero.
	 * Naturally, the LMM doesn't like this,
	 * so we have to split it appropriately here.
	 */
	if (end_va > start_va) {
		lmm_add_region(&malloc_lmm, &reg[0],
				(void*)start_va, end_va - start_va,
				flags, pri);
	} else {
		lmm_add_region(&malloc_lmm, &reg[0],
				(void*)start_va, 0xffffffff - start_va,
				flags, pri);
		lmm_add_region(&malloc_lmm, &reg[1],
				(void*)0, end_va,
				flags, pri);
	}
}

void phys_lmm_init(void)
{
#ifndef KNIT
	/* It wouldn't be good to add these regions more than once.  */
	if (inited) return;
	inited = 1;
#endif

	add(reg1mb, 0x00000000, 0x00100000, LMMF_1MB | LMMF_16MB, LMM_PRI_1MB);
	add(reg16mb, 0x00100000, 0x01000000, LMMF_16MB, LMM_PRI_16MB);
	add(reghigh, 0x01000000, 0xffffffff, 0, LMM_PRI_HIGH);

	phys_mem_max = 0;
	phys_mem_min = 0xffffffff;
}

