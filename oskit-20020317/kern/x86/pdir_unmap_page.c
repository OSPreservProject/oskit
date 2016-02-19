/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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

#include <oskit/x86/base_paging.h>

void pdir_unmap_page(oskit_addr_t pdir_pa, oskit_addr_t la)
{
	pd_entry_t *pde;
	pt_entry_t *pte;

	/* Find the page directory entry.  */
	pde = pdir_find_pde(pdir_pa, la);

	/* Find the page table entry and invalidate it.  */
	if (*pde & INTEL_PTE_VALID)
	{
		pte = ptab_find_pte(pde_to_pa(*pde), la);
		if (*pte & INTEL_PTE_VALID)
			*pte = 0;
	}
}

