/*
 * Copyright (c) 1996, 1998, 1999, 2001 University of Utah and the Flux Group.
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
 * Change the protection bits for a range of memory. This routine
 * translates from SVM mapping bits and other goo, to the machine
 * dependent bits and the pdir functions in the kernel. In this case
 * (arm32) the mapping of just the access control bits is easy since
 * we can represent all the bits we want in the PTE, without having to
 * munge them into shape.
 *
 * SVMLOCK should be held.
 */
int
svm_prot_range(oskit_addr_t va, oskit_size_t len, unsigned int prot)
{
	oskit_addr_t	pa;
	pt_entry_t	*pte;
	unsigned int	mbits;

	/*
	 * Do this in a page loop, finding the actual pte entry since
	 * pdir_prot_range does not preserve the mod/ref bits like I
	 * think it should. 
	 */
	while (len > 0) {
		pte  = pdir_find_pte(base_pdir_pa, va);

		if (!pte)
			panic("svm_prot_range: No PTE for 0x%x\n", va);
		
		pa    = pte_to_pa(*pte);
		mbits = pte_to_modebits(*pte);

#ifdef  DEBUG_SVM
		printf("svm_prot_range: "
		       "pte for 0x%x = 0x%x\n", (int)va, *(int *)pte);
#endif
		if (prot & SVM_PROT_WRITE)
			mbits = PT_AP(ARM32_AP_KRW) |
				(mbits & ARM32_PTE_CONTROL_MASK);
		else
			mbits = PT_AP(ARM32_AP_KR) |
				(mbits & ARM32_PTE_CONTROL_MASK);

		*pte = create_pte(pa, mbits);
		ftlbentry(va);

		va   += PAGE_SIZE;
		len  -= PAGE_SIZE;
	}
	return 0;
}
