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

void ptab_dump(oskit_addr_t ptab_pa, oskit_addr_t base_va)
{
	pt_entry_t *ptab = ptab_find_pte(ptab_pa, 0);
	unsigned ptn, ptm;

	for (ptn = 0; ptn < NPTES; ptn++)
	{
		/* Ignore invalid entries.  */
		if (! (((ptab[ptn] & ARM32_PTE_TYPE_MASK) ==
			ARM32_PTE_TYPE_BPAGE) ||
		       ((ptab[ptn] & ARM32_PTE_TYPE_MASK) ==
			ARM32_PTE_TYPE_SPAGE)))
			continue;

		/* Scan for a contiguous range of page mappings.  */
		for (ptm = ptn+1; ptm < NPTES; ptm++)
		{
			if (ptab[ptm] != ptab[ptm-1] + 4096)
				break;
		}

		printf("\t\t%08x-%08x -> %08x-%08x(%08x)%s%s%s%s\n",
			base_va + ptenum2va(ptn),
			base_va + ptenum2va(ptm)-1,
			pte_to_pa(ptab[ptn]),
			pte_to_pa(ptab[ptm-1]) + 4095,
		        (unsigned) ptab[ptn],
			ptab[ptn] & ARM32_PTE_TYPE_BPAGE ? " BPAGE" : "",
			ptab[ptn] & ARM32_PTE_TYPE_SPAGE ? " SPAGE" : "",
			ptab[ptn] & ARM32_PTE_BUFFERABLE ? " BUF" : "",
			ptab[ptn] & ARM32_PTE_CACHEABLE ? " CACHE" : "");

		ptn = ptm-1;
	}
}

