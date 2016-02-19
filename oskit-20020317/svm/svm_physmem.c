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
#include <oskit/lmm.h>

extern lmm_t		malloc_lmm;
extern oskit_size_t	svm_lowwater, svm_highwater;

/*
 * Ask the lmm for a phys page. Call the pageout code if one cannot
 * be found.
 */
oskit_addr_t
svm_alloc_physpage(void)
{
	oskit_addr_t	kvaddr, paddr;

	if (svm_physmem_avail() < svm_lowwater && svm_paging)
		svm_pageout();

	kvaddr = (oskit_addr_t)
		lmm_alloc_aligned(&malloc_lmm, PAGE_SIZE, 0, 12, 0);

	if (kvaddr == NULL)
		panic("svm_fault: No more physical memory");

	/*
	 * The malloc lmm is in terms of kernel virtual addresses.
	 */
	paddr = kvtophys(kvaddr);

	/*
	 * Undo the mapping that makes the phys page accessible through
	 * its kernel virtual address.
	 */
	svm_unmap_range(kvaddr, PAGE_SIZE);
	ftlbentry(kvaddr);

	return paddr;
}

/*
 * Return a phys page to the lmm.
 */
void
svm_dealloc_physpage(oskit_addr_t paddr)
{
	oskit_addr_t	kvaddr;

	/*
	 * The malloc lmm is in terms of kernel virtual addresses.
	 */
	kvaddr = phystokv(paddr);

	/*
	 * Must make the physical page accessible again through its
	 * kernel virtual address.
	 */
	if (svm_map_range(kvaddr, paddr, PAGE_SIZE,
			  PTE_MBITS_VALID|PTE_MBITS_RW))
		panic("svm_dealloc_physpage: Can't remap paddr 0x%x", paddr);

	lmm_free(&malloc_lmm, (void *) kvaddr, PAGE_SIZE);
}

/*
 * Return the amount of free phys memory available.
 *
 * NB: lmm_avail is a tad expensive to call too often.
 */
oskit_size_t
svm_physmem_avail(void)
{
	return lmm_avail(&malloc_lmm, 0);
}
