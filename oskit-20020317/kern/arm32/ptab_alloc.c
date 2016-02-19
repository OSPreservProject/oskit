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

#include <stdlib.h>
#include <string.h>

#include <oskit/lmm.h>
#include <oskit/arm32/base_paging.h>
#include <oskit/arm32/base_vm.h>
#include <oskit/arm32/proc_reg.h>

extern struct lmm malloc_lmm; /* XXX */

/*
 * Page tables are only 256 entries, or 1/4 of a page. We pack then 4
 * to a page though, since we need to adjust the access control bits
 * in the page tables for them, and it is better to do that for pages
 * that have just page tables in them.
 */
static	oskit_addr_t	latest_page;
static  int		latest_count;

int
ptab_alloc(oskit_addr_t *out_ptab_pa)
{
	oskit_addr_t	ptab_va;

	if (latest_page) {
		ptab_va = latest_page + (ARM32_PT_SIZE * latest_count);

		latest_count++;
		if (latest_count >= (PAGE_SIZE / ARM32_PT_SIZE)) {
			latest_page  = NULL;
			latest_count = 0;
		}
	}
	else {
		ptab_va = latest_page = (oskit_addr_t)
			lmm_alloc_aligned(&malloc_lmm, PAGE_SIZE,
					  0, PAGE_SHIFT, 0);
		latest_count = 1;
	}
	if (ptab_va == 0)
		return -1;

	/* Clear it out to make sure all entries are invalid.  */
	memset((void *) ptab_va, 0, ARM32_PT_SIZE);

	*out_ptab_pa = kvtophys(ptab_va);
	return 0;
}

/*
 * Fix the mapping for a page table. We do not want the PTEs cacheable
 * or bufferable. This avoids the cache flushing issue, which appears
 * to be a problem since I cannot get clean/flush entry to work!
 */
void
ptab_fix_mapping(oskit_addr_t pdir_pa, oskit_addr_t ptab_pa)
{
	oskit_addr_t	ptab_va = phystokv(ptab_pa);
	pd_entry_t      *pde    = pdir_find_pde(pdir_pa, ptab_va);
	pt_entry_t	*pte;
	unsigned int	pte_bits;

	/*
	 * Take into account initialization, when the memory being used
	 * for page tables might not be in the page directory yet. This
	 * will get fixed up in base_paging_load().
	 */
	if (! *pde)
		return;

	pte = ptab_find_pte(pde_to_pa(*pde), ptab_va);

	if (! *pte)
		return;

	pte_bits  = (unsigned int) *pte;
	pte_bits &= ~(ARM32_PTE_CACHEABLE|ARM32_PTE_BUFFERABLE);
	*pte      = (pt_entry_t) pte_bits;

	/*
	 * ACK! Why doesn't clean/flush entry work!
	 */
	arm32_cache_clean();
	drain_write_buffer();
	flush_tlb_ID();
}

