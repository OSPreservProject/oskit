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
#include <oskit/x86/proc_reg.h>

void pdir_unmap_range(oskit_addr_t pdir_pa, oskit_addr_t la, oskit_size_t size)
{
	while (size > 0)
	{
		pd_entry_t *pde = pdir_find_pde(pdir_pa, la);

		assert(*pde & INTEL_PDE_VALID);

		if (*pde & INTEL_PDE_SUPERPAGE)
		{
			/*
			 * A superpage was used to map an entire 4MB region.
			 * Just zero out the superpage entry in the PDE.
			 */
			assert(superpage_aligned(la));
			assert(size >= SUPERPAGE_SIZE);
			*pde = 0;
			la += SUPERPAGE_SIZE;
			size -= SUPERPAGE_SIZE;
		}
		else
		{
			pt_entry_t *pte;

			/* Find the page table for this 4MB region.  */
			assert(*pde & INTEL_PDE_VALID);
			pte = ptab_find_pte(pde_to_pa(*pde), la);

			/* Clear out the affected 4KB page mappings.  */
			do
			{
				/* Remove the page mapping.  */
				assert(*pte & INTEL_PTE_VALID);
				*pte = 0;

				/* Advance to the next page.  */
				pte++;
				la += PAGE_SIZE;
				size -= PAGE_SIZE;
			}
			while ((size > 0) && !superpage_aligned(la));
		}
	}
}

