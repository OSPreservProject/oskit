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

/*
 * Implementation of osenv memory interface. Together with osenv_mem.c
 * in this directory, override the default memory interface defined in
 * the dev directory. This implementation is basically the same, since
 * we are not doing anything fancy. Clients of the memory interface are
 * going to get back physical memory, which is already mapped into the
 * virtual address range (by virtue of how SVM works). The difference is
 * in how I/O memory is handled. mem_map_phys maps I/O space locations
 * into the virtual range so that device drivers can access them.
 */
#include <oskit/dev/dev.h>
#include <oskit/com/mem.h>
#include <oskit/com/services.h>
#include <oskit/machine/base_vm.h>
#include <oskit/machine/physmem.h>

#include "oskit_uvm_internal.h"

static oskit_mem_t *osenv_mem;

/*
 * Allocate memory.
 */
void *
osenv_mem_alloc(oskit_size_t size, osenv_memflags_t flags, unsigned align)
{
	void *p;

        if (osenv_mem == NULL) {
		if (oskit_lookup_first(&oskit_mem_iid, (void **)&osenv_mem))
			return NULL;
		if (osenv_mem == NULL)
			return NULL;
	}

	/*
	 * NOTE that osenv flags are the same as oskit_mem flags!
	 */

	if (align > 1) {
		p = oskit_mem_alloc_aligned(osenv_mem, size, flags, align);
	} else {
		p = oskit_mem_alloc(osenv_mem, size, flags);
	}

	return p;
}

/*
 * Free memory.
 */
void
osenv_mem_free(void *block, osenv_memflags_t flags, oskit_size_t size)
{
#ifdef MALLOCLOG
#undef free
#endif
	oskit_mem_free(osenv_mem, block, size, flags);
}

/*
 * Map physical memory into kernel virtual space.
 */
int
osenv_mem_map_phys(oskit_addr_t pa, oskit_size_t size, void **addr,
	int flags)
{
	return oskit_uvm_mem_map_phys(pa, size, addr, flags);
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
    pt_entry_t *pte;
    if (addr < phys_mem_max) {
	return ((oskit_addr_t)kvtophys(addr));
    }

    if (addr >= OSKIT_UVM_MINUSER_ADDRESS) {
	pte = vtopte(addr);
    } else {
	pte = kvtopte(addr);
    }
    return (*pte & INTEL_PDE_PFN);
}

oskit_addr_t
osenv_mem_get_virt(oskit_addr_t addr)
{
    if (addr < phys_mem_max) {
	return ((oskit_addr_t)kvtophys(addr));
    }
    panic(__FUNCTION__": cannot handle virtual range (0x%x)\n", addr);
}
