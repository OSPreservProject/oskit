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

#include <stdio.h>
#include <oskit/arm32/base_paging.h>

void pdir_dump(oskit_addr_t pdir_pa)
{
	pd_entry_t *pdir = pdir_find_pde(pdir_pa, 0);
	unsigned pdn, pdm;

	printf("Page directory at pa %08x (kva %08x):\n",
		pdir_pa, phystokv(pdir_pa));
	for (pdn = 0; pdn < NPDES; pdn++)
	{
		/* Ignore invalid entries.  */
		if ((pdir[pdn] & ARM32_PDE_TYPE_MASK)
		    == ARM32_PDE_TYPE_INVALID)
			continue;

		/* If this is a page table mapping, dump the page table.  */
		if (!(pdir[pdn] & ARM32_PDE_TYPE_SECTION))
		{
			oskit_addr_t ptab_pa = pde_to_pa(pdir[pdn]);

			printf("\t%08x-%08x -> "
			       "Page table at pa %08x (kva %08x)"
				"(%08x)%s%s\n",
				pdenum2va(pdn), pdenum2va(pdn+1)-1,
				ptab_pa, phystokv(ptab_pa),
			        (unsigned) pdir[pdn],
			        pdir[pdn]&ARM32_PDE_BUFFERABLE ? " BUF" : "",
			        pdir[pdn]&ARM32_PDE_CACHEABLE ? " CACHE" : "");
			ptab_dump(ptab_pa, pdenum2va(pdn));
			continue;
		}

		/* Scan for a contiguous range of superpage mappings.  */
		for (pdm = pdn+1; pdm < NPDES; pdm++)
		{
			if (pdir[pdm] != pdir[pdm-1] + 1*1024*1024)
				break;
		}

		printf("\t%08x-%08x -> %08x-%08x%s%s\n",
			pdenum2va(pdn), pdenum2va(pdm)-1,
			pde_to_pa(pdir[pdn]),
			pde_to_pa(pdir[pdm-1]) + 1*1024*1024 - 1,
		        pdir[pdn]&ARM32_PDE_BUFFERABLE ? " BUF" : "",
		        pdir[pdn]&ARM32_PDE_CACHEABLE ? " CACHE" : "");

		pdn = pdm-1;
	}
}

