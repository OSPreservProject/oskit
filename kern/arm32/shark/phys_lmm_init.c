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
#include <oskit/arm32/physmem.h>
#include <oskit/arm32/shark/base_vm.h>
#include <oskit/arm32/shark/phys_lmm.h>

static int inited;
static struct lmm_region reglow[2], reghigh[2];
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
	/* It woudn't be good to add these regions more than once.  */
	if (inited) return;
	inited = 1;

	/*
	 * Well, this is a pain. The Shark defines a very small DMA range
	 * at the beginning of physical memory. This is determined by
	 * the ofw_configisadma function, but I'll just hardwire that in
	 * until later. It ain't 16 megs either, but the LMMF flags are
	 * have pretty much leaked out all over the place, so just use
	 * them.
	 */
	add(reglow,  0x08000000, 0x08400000, LMMF_16MB, LMM_PRI_16MB);
	add(reghigh, 0x08400000, 0x10000000, 0, LMM_PRI_HIGH);

	phys_mem_max = 0;
	phys_mem_min = 0xffffffff;
}

