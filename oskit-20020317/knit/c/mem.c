/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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
 * lmm to osenv_mem adaptor
 */
#include <oskit/dev/dev.h>

#include <oskit/lmm.h>
#include <knit/c/lmm.h>
#include <oskit/machine/base_vm.h>
#include <oskit/machine/physmem.h>
#include <oskit/machine/phys_lmm.h>

#include "bitops.h"
#include "mallocf.h"

/*
 * Allocate memory.
 */
void *
osenv_mem_alloc(oskit_size_t size, osenv_memflags_t flags, unsigned align)
{
	lmm_flags_t lmm_flags = 0;
	void *p;

	if (flags & OSENV_ISADMA_MEM)
		lmm_flags |= LMMF_16MB;
	if (flags & OSENV_X861MB_MEM)
		lmm_flags |= LMMF_1MB;

        if (flags & OSENV_AUTO_SIZE) {
                if (align > 1) {
                        p = memalign_flags(size, lmm_flags, align);
                } else {
                        p = malloc_flags(size, lmm_flags);
                }
        } else { 
                if (align > 1) {
                        p = smemalign_flags(size, lmm_flags, align);
                } else {
                        p = smalloc_flags(size, lmm_flags);
                }
        }
	return p;
}

/*
 * Free memory.
 */
void
osenv_mem_free(void *block, osenv_memflags_t flags, oskit_size_t size)
{
        /* Ignores all flags except AUTO_SIZE */

        if (flags & OSENV_AUTO_SIZE) {
                free_flags(block);
        } else { 
                sfree_flags(block, size);
        }
}

/*
 * Map physical memory into kernel virtual space.
 */
int
osenv_mem_map_phys(oskit_addr_t pa, oskit_size_t size, void **addr,
	int flags)
{
	*addr = (void *)phystokv(pa);
	return (0);
}


/*
 * Return the amount of physical RAM in the machine
 */
oskit_addr_t
osenv_mem_phys_max(void)
{
	/*
	 * XXX How can this possibly be useful? It makes no sense on
	 * any machine in which memory does not start at zero, or is
	 * not contiguous.
	 */
	
	return phys_mem_max;
}

/*
 * Return the physical address
 */
oskit_addr_t
osenv_mem_get_phys(oskit_addr_t addr)
{
	return ((oskit_addr_t)kvtophys(addr));
}

oskit_addr_t
osenv_mem_get_virt(oskit_addr_t addr)
{
	return ((oskit_addr_t)phystokv(addr));
}
