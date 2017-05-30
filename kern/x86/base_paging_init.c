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
/*
 * Code to set up a basic paged memory environment on a PC.
 * Creates a single page directory, base_pdir,
 * which direct-maps all physical memory up to phys_mem_max (see phys_lmm.h).
 */

#include <oskit/debug.h>
#include <oskit/x86/base_paging.h>
#include <oskit/x86/pc/phys_lmm.h>

void base_paging_init(void)
{
	if (ptab_alloc(&base_pdir_pa))
		panic("Can't allocate kernel page directory");

	if (pdir_map_range(base_pdir_pa, 0, 0, round_superpage(phys_mem_max),
		INTEL_PDE_VALID | INTEL_PDE_WRITE | INTEL_PDE_USER))
		panic("Can't direct-map physical memory");
}

