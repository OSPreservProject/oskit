/*
 * Copyright (c) 2001 The University of Utah and the Flux Group.
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

#include "oskit_uvm_internal.h"
#include <oskit/x86/physmem.h>

/*
 * Map physical memory into kernel virtual space.
 * mimic of i386_mem_add_mapping() in bus_machdep.c
 */
extern int
oskit_uvm_mem_map_phys(oskit_addr_t pa, oskit_size_t size,
		       void **addr, int flags)
{
    oskit_addr_t endpa;
    vaddr_t va;
    pt_entry_t *pte;

    if (pa < phys_mem_max) {
	*addr = (void *)phystokv(pa);
	return 0;
    }

    pa = trunc_page(pa);
    endpa = i386_round_page(pa + size);

#ifdef DIAGNOSTIC
    if (endpa <= pa)
	panic(__FUNCTION__": overflow");
#endif

    va = uvm_km_valloc(kernel_map, endpa - pa);
    if (va == 0)
	return (OSKIT_ENOMEM);
    
    *addr = (void*)va;

    for (; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
	pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
	
	/*
	 * PG_N doesn't exist on 386's, so we assume that
	 * the mainboard has wired up device space non-cacheable
	 * on those machines.
	 */
	if (cpu_class != CPUCLASS_386) {
	    int npte;
	    pte = kvtopte(va);
	    npte = *pte;
	    if (flags & PHYS_MEM_NOCACHE) {
		npte |= INTEL_PTE_NCACHE;
	    } else {
		npte &= ~INTEL_PTE_NCACHE;
	    }
	    if (flags & PHYS_MEM_WRITETHROUGH) {
		npte |= INTEL_PTE_WTHRU;
	    } else {
		npte &= ~INTEL_PTE_WTHRU;
	    }
	    *pte = npte;
#ifdef LARGEPAGES
	    if (*pte & INTEL_PDE_SUPERPAGE)
		pmap_update_pg(va & PG_LGFRAME);
	    else
#endif
		pmap_update_pg(va);
	}
    }
    return 0;
}
