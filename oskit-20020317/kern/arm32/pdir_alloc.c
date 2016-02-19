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

#include <stdlib.h>
#include <string.h>

#include <oskit/lmm.h>
#include <oskit/arm32/base_paging.h>

extern struct lmm malloc_lmm; /* XXX */

int pdir_alloc(oskit_addr_t *out_pdir_pa)
{
	void *pdir_va = lmm_alloc_aligned(&malloc_lmm, ARM32_PD_SIZE,
					  0, ARM32_PD_SHIFT, 0);
	if (pdir_va == 0)
		return -1;

	/* Clear it out to make sure all entries are invalid.  */
	memset(pdir_va, 0, ARM32_PD_SIZE);

	*out_pdir_pa = kvtophys(pdir_va);
	return 0;
}

