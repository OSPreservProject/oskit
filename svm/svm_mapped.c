/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * Determine if a region is mapped.
 *
 * Address and len must be page aligned. 
 */
int
svm_mapped(oskit_addr_t addr, oskit_size_t len)
{
	oskit_addr_t	base = addr;

	/*
	 * Check for page alignment of both address and length.
	 */
	if (!page_aligned(base) || !page_aligned(len))
		return OSKIT_E_INVALIDARG;

	/*
	 * Make sure the region is one thats been allocated.
	 */
	if (! amm_find_gen(&svm_amm, &base,
			   len, AMM_ALLOCATED, AMM_ALLOCATED,
			   0, 0, AMM_EXACT_ADDR))
		return OSKIT_E_INVALIDARG;

	return 0;
}

