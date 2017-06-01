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
 * Change the mapping for a particular va. This changes the pa and the
 * mapping bits, translating from SVM mapping bits to the machine
 * dependent goo. In this case (x86) the mapping is easy since we can
 * represent all the bits we want in the PTE, without having to munge
 * them into shape.
 *
 * SVMLOCK should be held.
 */
int
svm_change_mapping(oskit_addr_t va, oskit_addr_t pa, unsigned int mbits)
{
	pt_entry_t	*pte;
	unsigned int	pte_bits;

	pte = pdir_find_pte(base_pdir_pa, va);

	if (!pte)
		return -1;

	/*
	 * Construct a valid PTE entry from the mapping_bits. All 4 sub
	 * pages get the same access permission bits. 
	 */
	pte_bits = PT_AP(PT_PTE2AP0(mbits)) |
		   (mbits & ARM32_PTE_CONTROL_MASK) |
		   ARM32_PTE_CACHEABLE|ARM32_PTE_BUFFERABLE;

	/*
	 * XXX Since there are no hardware ref/mod bits, set them now.
	 * This will break pagein/pageout/incore, but I'll deal with
	 * it later.
	 */
	mbits |= PTE_MBITS_REF|PTE_MBITS_MOD;
	sdir_map_page(va, mbits);

	*pte = create_pte(pa, pte_bits);

	ftlbentry(va);

	return 0;
}

