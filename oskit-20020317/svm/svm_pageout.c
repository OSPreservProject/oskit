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

extern int		svm_ptov_count;
extern oskit_size_t	svm_lowwater, svm_highwater;
int			svm_pageout_count;
int			svm_pageout_time;

/*
 * Choose pages for pageout. This uses a simplified clock sweep, choosing
 * non-referenced pages until the highwater mark is reached.
 */
void
svm_pageout(void)
{
	static int	lastptr = 0;
	int		i, count = 0;
	unsigned int	mbits;
	oskit_addr_t	vaddr, paddr;
	oskit_size_t	avail;

#ifdef  DEBUG_SVM_PAGEOUT
	printf("svm_pageout: "
	       "Avail memory is %d bytes\n", svm_physmem_avail());
#endif
	if ((avail = svm_physmem_avail()) > svm_lowwater)
		return;

	/*
	 * Look for non-null entries in the ptov table. Keep scanning
	 * until we reach the highwater mark.
	 */
	for (i = 0; i < svm_ptov_count; i++) {
		if ((vaddr = svm_ptov[lastptr]) != SVM_NULL_PTOV) {
			
			if (svm_find_mapping(vaddr, &paddr, &mbits))
				panic("svm_pageout: Invalid vaddr:0x%x",vaddr);

			/*
			 * Wired pages are skipped.
			 */
			if (mbits & PTE_MBITS_WIRED)
				goto nextone;

			/*
			 * Check reference bit and pageout the page.
			 * Otherwise, clear the reference bit and continue.
			 */
			if ((mbits & PTE_MBITS_REF) == 0) {
				svm_pageout_page(vaddr);
				count++;

				avail += PAGE_SIZE;
				if (avail > svm_highwater)
					goto done;
				
				goto nextone;
			}

			mbits &= ~PTE_MBITS_REF;
			svm_change_mapping(vaddr, paddr, mbits);
		}
 	 nextone:
		if (++lastptr == svm_ptov_count)
			lastptr = 0;
	}
	/*
	 * Could not find enough non-referenced pages. Start tossing out
	 * any pages we can.
	 */
	for (i = 0; i < svm_ptov_count; i++) {
		if ((vaddr = svm_ptov[lastptr]) != SVM_NULL_PTOV) {
			
			if (svm_find_mapping(vaddr, &paddr, &mbits))
				panic("svm_pageout: Invalid vaddr:0x%x",vaddr);

			/*
			 * Pageout non-wired pages.
			 */
			if ((mbits & PTE_MBITS_WIRED) == 0) {
				svm_pageout_page(vaddr);
				count++;
				
				avail += PAGE_SIZE;
				if (avail > svm_highwater)
					goto done;
			}
		}
		if (++lastptr == svm_ptov_count)
			lastptr = 0;
	}
   done:
	
#ifdef  DEBUG_SVM_PAGEOUT
	printf("svm_pageout: "
	       "Paged out %d pages. Avail memory is %d bytes\n",
	       count, svm_physmem_avail());
#endif
}

/*
 * Pageout a virtual page. I don't allocate a diskpage until needed,
 * and I don't remember them when the page is paged in. So, the MOD
 * bit is basically ignored. Pages are always written.
 *
 * NB: Pages are paged through the virtual address.
 */
void
svm_pageout_page(oskit_addr_t vaddr)
{
	oskit_addr_t	paddr;
	unsigned int	mbits;
	int		diskpage;
	oskit_u64_t	before, after;

	before = get_tsc();

	if (svm_find_mapping(vaddr, &paddr, &mbits))
		panic("svm_pageout_page: Invalid vaddr:0x%x", vaddr);

	if (mbits & PTE_MBITS_PAGED)
		panic("svm_pageout_page: vaddr:0x%x already paged!");

	/*
	 * Find a free disk page and send it to disk.
	 */
	diskpage = svm_getdiskpage();

#ifdef	DEBUG_SVM
	printf("svm_pageout_page: vaddr:0x%x paddr:0x%x db:%d\n",
	       vaddr, paddr, diskpage);
#endif
	/*
	 * Write from the virtual address!
	 */
	if (svm_swapwrite(diskpage, vaddr))
		panic("svm_pageout_page: Can't pageout page!");

	/*
	 * Change ptov and pte to reflect page now gone.
	 */
	SVM_PTOV(paddr) = SVM_NULL_PTOV;
	svm_dealloc_physpage(paddr);

	/*
	 * New mode is paged and invalid.
	 */
	mbits |=  PTE_MBITS_PAGED;
	mbits &= ~PTE_MBITS_VALID;
	svm_change_mapping(vaddr, (oskit_addr_t) ptob(diskpage), mbits);

	after  = get_tsc();
	if (after > before) {
		svm_pageout_time += ((int) (after - before)) / 200;
		svm_pageout_count++;
	}
}
