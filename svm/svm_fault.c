/*
 * Copyright (c) 1996, 1998, 1999, 2000 University of Utah and the Flux Group.
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
#include <oskit/c/strings.h>
#include <oskit/c/string.h>

/*
 * Page fault handler. If the fault is for a mapped page that is paged
 * out or unallocated, take care of it.
 *
 * If its a protection or otherwise unallocated reference, pass to the
 * SEGV handler if it is defined. Otherwise panic.
 */
int
svm_fault(oskit_addr_t vaddr, int eflags)
{
	oskit_addr_t	vpage;
	oskit_addr_t	paddr;
	unsigned int	mbits;
	int		status = SVM_FAULT_OKAY;

	SVM_LOCK();

#ifdef  DEBUG_SVM
	printf("svm_fault: 0x%x 0x%x\n", vaddr, eflags);
#endif

	vpage = trunc_page(vaddr);

	/*
	 * If address not mapped, go right to SEGV handler.
	 */
	if (svm_mapped(vpage, PAGE_SIZE)) {
		status = SVM_FAULT_PAGEFLT;
		goto segv;
	}

	/*
	 * Address is mapped. Is the page not even allocated (zero-fill)?
	 * If so, find a page and map it. Just return.
	 */
	if (svm_find_mapping(vpage, &paddr, &mbits))
		panic("svm_fault: Invalid mapping for 0x%x!", vpage);

	if (mbits & PTE_MBITS_ZFILL) {
		/*
		 * Zero fill. Allocate a page and map it.
		 */
		paddr = svm_alloc_physpage();

#ifdef  DEBUG_SVM
		printf("svm_fault: "
		       "Mapping vaddr 0x%x to paddr 0x%x\n", vpage, paddr);
#endif
		/*
		 * Plug into ptov table if paging is enabled.
		 */
		if (svm_paging)
			SVM_PTOV(paddr) = vpage;

		/*
		 * Change the mapping. Must be writable initially, since
		 * it might be a read-only page.
		 */
		svm_change_mapping(vpage, paddr,
				   PTE_MBITS_VALID|PTE_MBITS_RW);

		/*
		 * Zero the page through the virtual address.
		 */
		memset((void *)vpage, 0, PAGE_SIZE);

		/*
		 * Restore original mapping.
		 */
		mbits &= ~PTE_MBITS_ZFILL;
		mbits |= PTE_MBITS_VALID;
		svm_change_mapping(vpage, paddr, mbits);

		goto done;
	}

	/*
	 * If there is a paddr, it might be a disk block number. Call the
	 * pagein code to bring it in.
	 */
	if (mbits & PTE_MBITS_PAGED) {
		svm_pagein_page(vpage);
		goto done;
	}

	/*
	 * Region is mapped and has a valid physical translation. Must
	 * be a protection violation.
	 */
	status = SVM_FAULT_PROTFLT;
	
   segv:
	SVM_UNLOCK();
	if (svm_segv_handler)
		return (*svm_segv_handler)(vaddr, eflags);

	return status;

   done:
	SVM_UNLOCK();
	return SVM_FAULT_OKAY;
}
