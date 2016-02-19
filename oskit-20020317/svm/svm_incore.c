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
 * Return information about each page in a region.
 * The region does not have to be part of a previously
 * For each page in the region, one byte in `pagemap' is
 * written indicating whether the page is in core and
 * if so, whether it has been referenced or modified
 * since the last svm_incore call or remapping.
 *
 * Address and len must be page aligned.
 */
int
svm_incore(oskit_addr_t addr, oskit_size_t len, char *pagemap)
{
	oskit_addr_t	base = addr;
	char		*p = pagemap;
	oskit_addr_t	paddr;
	unsigned int	mbits;

	/*
	 * Check for page alignment of both address and length.
	 */
	if ((base && !page_aligned(base)) || !page_aligned(len))
		return OSKIT_E_INVALIDARG;

	/* XXX bad scene if we fault writing pagemap */
	SVM_LOCK();

	while (len) {
		if (svm_find_mapping(addr, &paddr, &mbits)) {
			*p = 0;
		}
		else {
#ifdef  DEBUG_SVM
			printf("svm_incore: "
			       "vaddr:0x%x paddr:0x%x bits:0x%x\n",
			       (int) addr, (int) paddr, mbits);
#endif
			if (mbits & PTE_MBITS_VALID) {
				*p = SVM_INCORE_INCORE;
				if (mbits & PTE_MBITS_REF)
					*p |= SVM_INCORE_REFERENCED;
				if (mbits & PTE_MBITS_MOD)
					*p |= SVM_INCORE_MODIFIED;
				if (mbits & (PTE_MBITS_REF|PTE_MBITS_MOD)) {
					mbits &= ~(PTE_MBITS_REF|
						   PTE_MBITS_MOD);
					svm_change_mapping(addr, paddr, mbits);
				}
			}
			else
				*p = 0;
		}

		addr += PAGE_SIZE;
		len  -= PAGE_SIZE;
		++p;
	}

	SVM_UNLOCK();
	return 0;
}
