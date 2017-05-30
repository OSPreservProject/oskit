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

#include <svm/svm_internal.h>

/*
 * Map a range of memory. This routine translates from SVM mapping
 * bits and other goo, to the machine dependent bits and the pdir
 * functions in the kernel. In this case (x86) the mapping is easy
 * since we can represent all the bits we want in the PTE, without
 * having to munge them into shape.
 *
 * SVMLOCK should be held.
 */
int
svm_map_range(oskit_addr_t va, oskit_addr_t pa,
	      oskit_size_t size, unsigned int mapping_bits)
{
	oskit_addr_t	vaddr, paddr;
	pt_entry_t	pte;

#ifdef  DEBUG_SVM
	printf("svm_map_range: va:0x%x pa:0x%x size:%d bits:0x%x\n",
	       pa, va, size, mapping_bits);
#endif
	vaddr = va;

	if (mapping_bits & PTE_MBITS_ZFILL) {
		paddr = 0;
		pte   = create_pte(0, mapping_bits);
	}
	else {
		paddr = pa;
		pte   = create_pte(paddr, mapping_bits);
	}

	while (size > 0) {
		if (pdir_map_page(base_pdir_pa, vaddr, pte))
			panic("svm_map_range: Can't map physical memory");

		if (! (mapping_bits & PTE_MBITS_ZFILL)) {
			paddr += PAGE_SIZE;
			pte   = create_pte(paddr, mapping_bits);
		}

		size  -= PAGE_SIZE;
		vaddr += PAGE_SIZE;
	}
	
	return 0;
}

