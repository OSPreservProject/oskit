/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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

#include "svm_internal.h"

/*
 * Unmap a region.
 *
 * Address and len must be page aligned.
 */
int
svm_dealloc(oskit_addr_t addr, oskit_size_t len)
{
	oskit_addr_t	base = addr;

	/*
	 * Check for page alignment of both address and length.
	 */
	if ((base && !page_aligned(base)) || !page_aligned(len))
		return OSKIT_E_INVALIDARG;

	SVM_LOCK();

	/*
	 * Make sure the region is one thats been allocated, but not part
	 * of the SVM reserved region.
	 */
	if (! amm_find_gen(&svm_amm, &base,
			   len, AMM_ALLOCATED, AMM_ALLOCATED|SVM_PROT_PHYS,
			   0, 0, AMM_EXACT_ADDR)) {
		SVM_UNLOCK();
		return OSKIT_E_INVALIDARG;
	}

	if (amm_deallocate(&svm_amm, addr, len))
		panic("svm_dealloc: Can't AMM unmap range");

	/*
	 * Do this in a page loop, finding the actual pte info since
	 * we need to know the physical page number so it can be given
	 * back to the lmm.
	 */
	while (len > 0) {
		oskit_addr_t	pa;
		unsigned int	obits;

		if (svm_find_mapping(addr, &pa, &obits))
			panic("svm_protect: No PTE for 0x%x\n", addr);
		
#ifdef  DEBUG_SVM
		printf("svm_dealloc: Mapping for 0x%x = pa:0x%x bits:0x%x\n",
		       addr, pa, obits);
#endif
		/*
		 * Unmap the virtual page first.
		 */
		if (obits & PTE_MBITS_VALID) { /* XXX ? */
			svm_unmap_range(addr, PAGE_SIZE);
			ftlbentry(addr);
		}

		/*
		 * The phys page might be a real physical address, or it
		 * might be a zfill, or it might be a disk block number.
		 */
		if (obits & (PTE_MBITS_ZFILL|PTE_MBITS_PAGED))
			goto nextpage;

		svm_dealloc_physpage(pa);

		/*
		 * Remove translation from the ptov table if paging enabled.
		 */
		if (svm_paging)
			SVM_PTOV(pa) = SVM_NULL_PTOV;

	nextpage:
		addr += PAGE_SIZE;
		len  -= PAGE_SIZE;
	}

	SVM_UNLOCK();
	return 0;
}
