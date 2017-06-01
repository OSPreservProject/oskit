/*
 * Copyright (c) 1998 University of Utah and the Flux Group.
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

void pdir_clean_range(oskit_addr_t pdir_pa, oskit_addr_t la, oskit_size_t size)
{
	int i, j;
	pd_entry_t *pde;
	pt_entry_t *pte;
	oskit_addr_t ptab_pa;

	/* Loop over indicated PDEs looking for valid page table pages.  */
	pde = pdir_find_pde(pdir_pa, la);
	for (i = round_page(size) >> PDESHIFT; i > 0; i--, pde++)
	{
		/* Skip invalid or superpage PDEs.  */
		if ((*pde & (INTEL_PDE_VALID|INTEL_PDE_SUPERPAGE))
		    != INTEL_PDE_VALID)
			continue;

		/* Valid page table, determine if all entries are invalid.  */
		ptab_pa = pde_to_pa(*pde);
		pte = ptab_find_pte(ptab_pa, 0);
		for (j = 0; j < 1024; j++, pte++)
			if (*pte & INTEL_PTE_VALID)
				break;

		/* If all are invalid, free up the page table page.  */
		if (j == 1024)
		{
			*pde = 0;
			ptab_free(ptab_pa);
		}
	}
}
