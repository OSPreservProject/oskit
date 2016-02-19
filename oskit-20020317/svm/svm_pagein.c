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

int			svm_pagein_count, svm_pagein_time;

/*
 * Page in a page from disk.
 *
 * NB: Pages are paged through the virtual address.
 */
void
svm_pagein_page(oskit_addr_t vaddr)
{
	oskit_addr_t	paddr;
	unsigned int	mbits;
	int		diskpage;
	oskit_u64_t	before, after;

	before  = get_tsc();

	if (svm_find_mapping(vaddr, &paddr, &mbits))
		panic("svm_pagein_page: Invalid vaddr:0x%x", vaddr);

	if ((mbits & PTE_MBITS_PAGED) == 0)
		panic("svm_pagein_page: Page not paged! vaddr:0x%x", vaddr);

	diskpage = btop(paddr);

	/*
	 * Allocate a new physical page and plug into the ptov table.
	 */
	paddr = svm_alloc_physpage();
	SVM_PTOV(paddr) = vaddr;

#ifdef  DEBUG_SVM
	printf("svm_pagein_page: "
	       "Paging in vaddr 0x%x to paddr 0x%x from diskpage:%d\n",
	       vaddr, paddr, diskpage);
#endif

	/*
	 * Change the mapping. Must be writable initially.
	 */
	svm_change_mapping(vaddr, paddr, PTE_MBITS_VALID|PTE_MBITS_RW);

	/*
	 * and read the data in from disk through the virtual adress.
	 */
	if (svm_swapread(diskpage, vaddr))
		panic("svm_pagein_page: "
		      "Can't pagein page! vaddr:0x%x paddr:0x%x diskpage:%d",
		      vaddr, paddr, diskpage);

	/*
	 * success. Free the disk page.
	 */
	svm_freediskpage(diskpage);

	/*
	 * Restore original mapping. Clear MOD/REF bits.
	 */
	mbits |= PTE_MBITS_VALID;
	mbits &= ~(PTE_MBITS_PAGED|PTE_MBITS_MOD|PTE_MBITS_REF);
	svm_change_mapping(vaddr, paddr, mbits);

	after  = get_tsc();
	if (after > before) {
		svm_pagein_time += ((int) (after - before)) / 200;
		svm_pagein_count++;
	}
}

