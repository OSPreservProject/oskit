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

#include <oskit/arm32/base_paging.h>

int pdir_map_page(oskit_addr_t pdir_pa, oskit_addr_t va, pt_entry_t mapping)
{
	pd_entry_t *pde;
	pt_entry_t *pte;

	/* Find the page directory entry.  */
	pde = pdir_find_pde(pdir_pa, va);

	/* Find the page table entry,
	   creating the page table if necessary.  */
	if (!(*pde & ARM32_PDE_TYPE_PAGETBL))
	{
		oskit_addr_t ptab_pa;
		int rc;

		/* Allocate a new page table.  */
		rc = ptab_alloc(&ptab_pa);
		if (rc)
			return rc;
		ptab_fix_mapping(pdir_pa, ptab_pa);

		/* Set the page directory entry to point to it.  */
		*pde = pa_to_pde(ptab_pa) | ARM32_PDE_TYPE_PAGETBL;
	}
	pte = ptab_find_pte(pde_to_pa(*pde), va);

	/* Insert the mapping.
	   There are no access/dirty bits in the PTE to worry about. */
	*pte = mapping;

	return 0;
}

