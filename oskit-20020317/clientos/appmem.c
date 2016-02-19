/*
 * Copyright (c) 1995-1996, 1998, 1999 University of Utah and the Flux Group.
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
 * This entrypoint allows the memory object to be used independent of
 * the already initialized malloc_lmm in the kernel library.
 */

#include <oskit/types.h>
#include <oskit/lmm.h>
#include <oskit/com/mem.h>
#include <oskit/machine/phys_lmm.h>

/* One region only */ 
static struct lmm_region region;

oskit_mem_t *
oskit_appmem_init(void *base, oskit_size_t size)
{
	/*
	 * Create one big LMM region covering the possible heap area.
	 */
	lmm_add_region(&malloc_lmm, &region, base, size, 0, 0);
	
	return oskit_mem_init();
}
