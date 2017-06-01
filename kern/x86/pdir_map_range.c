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

#include <oskit/debug.h>
#include <oskit/page.h>
#include <oskit/x86/base_cpu.h>
#include <oskit/x86/base_paging.h>

int pdir_map_range(oskit_addr_t pdir_pa, oskit_addr_t la, oskit_addr_t pa,
		   oskit_size_t size, pt_entry_t mapping_bits)
{
	assert(mapping_bits & INTEL_PTE_VALID);
	assert(!(mapping_bits & INTEL_PTE_PFN));

	while (size > 0)
	{
		pd_entry_t *pde = pdir_find_pde(pdir_pa, la);

		/* Use a 4MB page if we can.  */
		if (superpage_aligned(la) && superpage_aligned(pa)
		    && (size >= SUPERPAGE_SIZE)
		    && (base_cpuid.feature_flags & CPUF_4MB_PAGES))
		{
			assert(!(*pde & INTEL_PDE_VALID));
			/* XXX what if an empty page table exists
			   from previous finer-granularity mappings? */
			*pde = pa | mapping_bits | INTEL_PDE_SUPERPAGE;
			la += SUPERPAGE_SIZE;
			pa += SUPERPAGE_SIZE;
			size -= SUPERPAGE_SIZE;
		}
		else
		{
			pt_entry_t *pte;

			/* Find the page table, creating one if necessary.  */
			if (!(*pde & INTEL_PDE_VALID))
			{
				oskit_addr_t ptab_pa;
				int rc;

				/* Allocate a new page table.  */
				rc = ptab_alloc(&ptab_pa);
				if (rc)
					return rc;

				/* Set the pde to point to it.  */
				*pde = pa_to_pte(ptab_pa)
					| INTEL_PDE_VALID | INTEL_PDE_USER
					| INTEL_PDE_WRITE;
			}
			assert(!(*pde & INTEL_PDE_SUPERPAGE));
			pte = ptab_find_pte(pde_to_pa(*pde), la);

			/* Use normal 4KB page mappings.  */
			do
			{
				assert(!(*pte & INTEL_PTE_VALID));

				/* Insert the mapping.  */
				*pte = pa | mapping_bits;

				/* Advance to the next page.  */
				pte++;
				la += PAGE_SIZE;
				pa += PAGE_SIZE;
				size -= PAGE_SIZE;
			}
			while ((size > 0) && !superpage_aligned(la));
		}
	}

	return 0;
}

