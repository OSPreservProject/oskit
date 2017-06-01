/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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

#include <svm/svm_internal.h>

/*
 * Map physical memory into kernel virtual space.
 */
int
svm_mem_map_phys(oskit_addr_t pa, oskit_size_t size, void **addr, int flags)
{
	if ((pa < phys_mem_min || pa >= phys_mem_max) && svm_enabled) {
		printf("osenv_mem_map_phys: %x %d %x\n", pa, size, flags);

		if (pdir_map_range(base_pdir_pa, pa, pa, round_page(size),
				   PT_AP(ARM32_AP_KRWURW) |
				   ARM32_PTE_TYPE_SPAGE))
			panic("osenv_mem_map_phys: Can't map physical memory");

		*addr = (void *)pa;
	}
	else
		*addr = (void *)phystokv(pa);

	return 0;
}
