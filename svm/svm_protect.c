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
 * Protect a region. The region does not have to be part of a previously
 * allocation. This allows arbitrary protections within the applications
 * data space.
 *
 * Address and len must be page aligned. 
 */
int
svm_protect(oskit_addr_t addr, oskit_size_t len, int prot)
{
	oskit_addr_t	base = addr;

	/*
	 * Check for page alignment of both address and length.
	 */
	if ((base && !page_aligned(base)) || !page_aligned(len))
		return OSKIT_E_INVALIDARG;

	/*
	 * Allow READ or READ/WRITE only.
	 */
	if (prot != SVM_PROT_READ && prot != (SVM_PROT_READ|SVM_PROT_WRITE))
		return OSKIT_E_INVALIDARG;

	SVM_LOCK();

	/*
	 * Make sure the region is one that we know about.
	 */
	if (! amm_find_gen(&svm_amm, &base,
			   len, AMM_ALLOCATED, AMM_ALLOCATED,
			   0, 0, AMM_EXACT_ADDR)) {
		SVM_UNLOCK();
		return OSKIT_E_INVALIDARG;
	}

	svm_prot_range(addr, len, prot);
		
	SVM_UNLOCK();
	return 0;
}

