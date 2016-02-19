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

#include <oskit/x86/pc/phys_lmm.h>
#include <oskit/x86/eflags.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/pc/phys_lmm.h>
#include <oskit/x86/pc/base_real.h>
#include <oskit/x86/base_stack.h>
#include <oskit/x86/trap.h>
#include <oskit/x86/seg.h>
/* these symbols are defined in seg.h but also defined in
   objinclude/machine/segments.h later */
#undef SEL_LDT
#undef ISPL
#undef USERMODE
#undef KERNELMODE

#include <oskit/x86/base_tss.h>
#include <oskit/x86/base_idt.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/lmm.h>
/* the OSKit and NetBSD define cpu_info differently */
#define cpu_info oskit_cpu_info
#include <oskit/x86/base_cpu.h>
#undef cpu_info
#include "oskit_uvm_internal.h"

/*
 * Combine address and mode bits into a PTE.
 */
#define create_pte(a, m)	((pt_entry_t) (((oskit_addr_t) (a)) | (m)))

static void load_physmem(void);
static void protect_kernel(void);

/*
 * Initialize machine dependent part of UVM
 */
extern void
oskit_uvm_machdep_init()
{
    /*
     * Setup variables used in UVM and NetBSD codes
     */
    cpu_feature = base_cpuid.feature_flags;
    switch (base_cpuid.family) {
    case CPU_FAMILY_386:
	cpu_class = CPUCLASS_386;
	break;
    case CPU_FAMILY_486:
	cpu_class = CPUCLASS_486;
	break;
    case CPU_FAMILY_PENTIUM:
	cpu_class = CPUCLASS_586;
	break;
    case CPU_FAMILY_PENTIUM_PRO: /* FALLTHROUGH */
    default:
	cpu_class = CPUCLASS_686;
	break;
    }

    /*
     * Do not use superpage for kernel pages.  
     * XXX: If we have 4MB data section alignment, we can enable this again.
     */
    base_cpuid.feature_flags &= ~CPUF_4MB_PAGES;

    /*
     * Setup the initial page table.
     * UVM assumes we are running on already page translation enabled.
     * We don't use base_paging_init() because base_paging_init() initialize
     * unnecessary ptes.
     */
    if (ptab_alloc(&base_pdir_pa)) {
	panic("Can't allocate kernel page directory");
    }
    if (pdir_map_range(base_pdir_pa, 0, 0, round_page(phys_mem_max),
		       INTEL_PDE_VALID | INTEL_PDE_WRITE)) {
	panic("Can't direct-map physical memory");
    }
    base_paging_load();

    /*
     * nkpde is a pmap variable that contains the # of page directory entries
     * assigned for the kernel at boot time.  Since base_pagint_init()
     * maps all physical memory, we use phys_mem_max.
     */
    nkpde = ((long)round_page(phys_mem_max) >> PDESHIFT) + 1;

    /*
     * Map page table recursively (PTE_BASE)
     */
    {
	pd_entry_t *pde = pdir_find_pde(base_pdir_pa, (oskit_addr_t)PTE_BASE);
	*pde = create_pte(base_pdir_pa, INTEL_PDE_VALID|INTEL_PDE_WRITE);
    }

    printf("UVM: page directory pa = %x, PTE_BASE = %p\n", 
	   base_pdir_pa, PTE_BASE);

    /*
     * Set WP bit of CR0 to protect read only user page.
     */
    set_cr0(get_cr0() | CR0_WP);

    /*
     * Set UVM page size
     */
    uvm_setpagesize();

    /*
     * Load physical memory into UVM
     */
    load_physmem();

    /*
     * Set curpcb.
     */
    curpcb = &oskit_uvm_user.u_pcb;

    /*
     * Initialize pmap.  kva_begin is beginning of the kernel virtual
     * address space used for page tables, kernel malloc and so on.
     */
    {
	oskit_addr_t kva_begin;
	
	kva_begin = round_page((phys_mem_max < VM_MIN_KERNEL_VIRTUAL_AVAIL ?
		phys_mem_max : VM_MIN_KERNEL_VIRTUAL_AVAIL));

	printf("UVM: kernel VA starts at 0x%x\n", kva_begin);
	pmap_bootstrap(kva_begin);
    }

    /*
     * Set up our page fault handler
     */
    base_trap_handlers[T_PAGE_FAULT] = oskit_uvm_pfault_handler;

    /*
     * Protect the kernel itself
     */
    protect_kernel();

    /*
     * Prepare double fault handler
     */
    oskit_uvm_redzone_init();
}

/*
 * Load physical memory into UVM system
 */
static void
load_physmem()
{
    oskit_addr_t seg_start, seg_end;
    oskit_size_t avail;
    oskit_size_t uvmmem;
    oskit_size_t alloced = 0;
    int search_lower1m = 0;
    
    avail = lmm_avail(&malloc_lmm, 0);
    /* allocate 70% of the physical memory for UVM */
    uvmmem = trunc_page(avail / 10 * 7);
#if 0
    /* just for debugging the swap code */
    if (uvmmem > 4096*1024) {
	uvmmem = 4096*1024;
    }
#endif
    printf("UVM: available memory 0x%lx bytes\n", (long)avail);

    /* first scan skips the lower 1MB memory */
    while (uvmmem > 0) {
	oskit_addr_t maxblkaddr = 0;
	oskit_size_t maxblksize = 0;
	oskit_addr_t addr;
	oskit_size_t size;
	lmm_flags_t flags;

	/* Find the largest free block */
	for (addr = 0 ;
	     (lmm_find_free(&malloc_lmm, &addr, &size, &flags), size != 0) ;
	     addr = round_page(addr + size)) {
	    XPRINTF(OSKIT_DEBUG_INIT,
		    "UVM: found free memory 0x%lx .. 0x%lx\n", (long)addr, 
		    (long)addr + size - 1);
	    /* ignore lower memory at the first scan */
	    if ((flags & LMMF_1MB) && !search_lower1m) {
		continue;
	    }
	    if (size > maxblksize) {
		maxblksize = size;
		maxblkaddr = addr;
	    }
	}

	/*
	 * Register physical memory into UVM
	 */
	/* If addr is not page-aligned, resize the size */
	if ((maxblkaddr & PAGE_MASK)) {
	    maxblksize -= (PAGE_SIZE - (maxblkaddr & PAGE_MASK));
	}
	maxblksize = trunc_page(maxblksize);

	/*
	 * If we could not find a free memory block or enough big memory block,
	 * try lower 1m area. if it does not work, give up.
	 */
	if (maxblkaddr == 0 || maxblksize == 0) {
	    if (!search_lower1m) {
		search_lower1m = 1;
		continue;
	    }
	    break;
	}
	if (maxblksize > uvmmem) {
	    maxblksize = uvmmem;
	}
	seg_start = (oskit_addr_t)lmm_alloc_gen(&malloc_lmm, maxblksize, 
						flags, PAGE_SHIFT, 0,
						maxblkaddr,
						0xffffffff - maxblkaddr);
	assert(seg_start != 0);
	seg_end = seg_start + maxblksize;
	printf("UVM: loaded physical memory block [0x%x - 0x%x]\n",
	       seg_start, seg_end - 1);
	uvm_page_physload(atop(seg_start), atop(seg_end), atop(seg_start),
			  atop(seg_end), 0);
	uvmmem -= maxblksize;
	alloced += maxblksize;
    }

    /*
     * physmem is a variable in the NetBSD kernel. (machdep.c)
     */
    physmem = alloced / PAGE_SIZE;
    printf("UVM: %d KB (%d pages) reserved\n", alloced / 1024, physmem);
}


static void
protect_kernel()
{
    extern int	enable_gdb;	/* XXX */
    extern char		_start[], end[], start_of_data[];
    vaddr_t start_addr = (unsigned) _start & ~(PAGE_SIZE - 1);
	
    /*
     * Map lower part of memory (ROM and I/O).
     */
#if 0
    XPRINTF(OSKIT_DEBUG_INIT, "Mapping 0x%x to 0x%x\n", 0, (int) start_addr);
    /* We use pmap_protect rather than uvm_map_protect for protecting
       the kernel space because uvm_map_protect does not work when
       nothing is mapped onto the map. */
    pmap_protect(pmap_kernel(), 0, (vaddr_t)start_addr, VM_PROT_NONE);
#endif

    /*
     * Protect the text segment as read-only.
     */
    XPRINTF(OSKIT_DEBUG_INIT, "Protecting 0x%x to 0x%x\n",
	    (int) start_addr, (int)start_of_data);
    pmap_protect(pmap_kernel(), (vaddr_t)start_addr,
		 (vaddr_t) start_of_data,
		 VM_PROT_READ|VM_PROT_EXECUTE|
		 (enable_gdb ? VM_PROT_WRITE : 0));

    /*
     * Protect the data segment as read/write.
     */
    XPRINTF(OSKIT_DEBUG_INIT, "Protecting 0x%x to 0x%x\n",
	    (int) start_of_data, round_page(end));
    pmap_protect(pmap_kernel(), (vaddr_t) start_of_data,
		 (vaddr_t) round_page(end),
		 VM_PROT_READ|UVM_PROT_WRITE);
    
    /*
     * Protect the bottom page of the kernel stack.
     * XXX: should reclaim the page.
     */
    XPRINTF(OSKIT_DEBUG_INIT, "Mapping 0x%x to 0x%x\n", (int) base_stack_start,
	    (int) base_stack_start + PAGE_SIZE);
    pmap_protect(pmap_kernel(), (vaddr_t) base_stack_start,
		 (vaddr_t) base_stack_start + PAGE_SIZE, VM_PROT_READ);

    /*
     * XXX: We do not touch the rest of memory.  Maybe lmm needs write
     * access to them.
     */
}
