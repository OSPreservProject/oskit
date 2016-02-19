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

#include "svm_internal.h"

/*
 * Allocate a region. Allow the caller to request a specific location,
 * but fail if that location is not available.
 *
 * Address and len must be page aligned. 
 */
int
svm_alloc(oskit_addr_t *addrp, oskit_size_t len, int prot, int flags)
{
	oskit_addr_t	base  = *addrp;
	oskit_error_t	error = 0;
	oskit_addr_t	vaddr;
	oskit_size_t    tmp = len;
	unsigned int	mbits;

	/*
	 * Check for page alignment of both address and length.
	 */
	if ((base && !page_aligned(base)) || !page_aligned(len))
		return OSKIT_E_INVALIDARG;

	/*
	 * Allow READ or READ/WRITE only.
	 */
	if ((prot & SVM_PROT_READ) == 0)
		return OSKIT_E_INVALIDARG;

	SVM_LOCK();

	/*
	 * Now try to allocate the region from the AMM, checking first
	 * if the request is "fixed."
	 */
	if (base && (flags & SVM_ALLOC_FIXED)) {
		struct amm_entry *entry;

		entry = amm_find_gen(&svm_amm, &base,
				     len, AMM_FREE, -1, 0, 0, AMM_EXACT_ADDR);
		if (entry == 0) {
			SVM_UNLOCK();
			return OSKIT_E_OUTOFMEMORY;
		}
	}

	if ((error = amm_allocate(&svm_amm, &base, len, prot)) != 0) {
		SVM_UNLOCK();
		return error;
	}

#ifdef  DEBUG_SVM
	printf("svm_alloc: 0x%x\n", base);
#endif

	/*
	 * Don't bother to allocate memory. That will happen when the
	 * page is first referenced. Just set up the mappings and mark
	 * them as zfill so they fault.
	 */
	mbits = PTE_MBITS_ZFILL |
		((prot & SVM_PROT_WIRED) ? PTE_MBITS_WIRED : 0) |
		((prot & SVM_PROT_WRITE) ? PTE_MBITS_RW : PTE_MBITS_R);

	if (svm_map_range(base, 0, len, mbits))
		panic("svm_alloc: Can't map physical memory");

	SVM_UNLOCK();

	/*
	 * Well, if the region was created as wired, go through faulting
	 * in the pages.
	 */
	if (prot & SVM_PROT_WIRED) {
		vaddr = (oskit_addr_t) base;
		tmp   = len;
		while (tmp > 0) {
			svm_fault(vaddr, 0);
			tmp   -= PAGE_SIZE;
			vaddr += PAGE_SIZE;
		}
	}
	
	*addrp = base;
	return 0;
}

