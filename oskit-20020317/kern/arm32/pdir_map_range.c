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

#include <oskit/debug.h>
#include <oskit/page.h>
#include <oskit/arm32/base_cpu.h>
#include <oskit/arm32/base_paging.h>

#if 0
#define DPRINTF(fmt, args... ) printf(__FUNCTION__  ":" fmt , ## args)
#else
#define DPRINTF(fmt, args... )
#endif

int pdir_map_range(oskit_addr_t pdir_pa, oskit_addr_t va, oskit_addr_t pa,
		   oskit_size_t size, pt_entry_t mapping_bits)
{
	assert(mapping_bits & ARM32_PTE_TYPE_MASK);
	assert(!(mapping_bits & ARM32_PTE_PFN_SPAGE));

	while (size > 0)
	{
		pd_entry_t *pde = pdir_find_pde(pdir_pa, va);
		pt_entry_t *pte;

		/*
		 * Use only small pages in this implementation!
		 */

		/* Find the page table, creating one if necessary.  */
		if (!(*pde & ARM32_PDE_TYPE_PAGETBL))
		{
			oskit_addr_t ptab_pa;
			int rc;

			/* Allocate a new page table.  */
			rc = ptab_alloc(&ptab_pa);
			if (rc)
				return rc;
			ptab_fix_mapping(pdir_pa, ptab_pa);

			/* Set the pde to point to it.  */
			*pde = pa_to_pde(ptab_pa) | ARM32_PDE_TYPE_PAGETBL;
		}

		assert(!(*pde & ARM32_PDE_TYPE_SECTION));
		pte = ptab_find_pte(pde_to_pa(*pde), va);

		DPRINTF("0x%x 0x%x\n", pde_to_pa(*pde), va2ptenum(va));

		DPRINTF("0x%x 0x%x 0x%x\n", pde, *pde, pte);
		
		/* Use normal 4KB page mappings.  */
		do
		{
			assert(!(*pte & ARM32_PTE_TYPE_MASK));

			/* Insert the mapping.  */
			*pte = pa | mapping_bits;

			/* Advance to the next page.  */
			pte++;
			va += PAGE_SIZE;
			pa += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
		while ((size > 0) && !superpage_aligned(va));
	}

	return 0;
}

