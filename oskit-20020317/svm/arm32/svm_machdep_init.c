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

#include <svm/svm_internal.h>

/*
 * The software bits page directory.
 */
unsigned char		**svm_sdir;

extern int	enable_gdb;	/* XXX */

void
svm_machdep_init(void)
{
	extern char	_start[], start_of_data[], end[];
	oskit_addr_t	start_addr;
	oskit_addr_t	svm_sdir_pa;
	
	base_paging_init();

	if (pdir_alloc(&svm_sdir_pa))
		panic("Can't allocate software bits directory");

	/*
	 * Convert it to a kv address since the hardware never sees this.
	 */
	svm_sdir = (unsigned char **) phystokv(svm_sdir_pa);

	start_addr = (unsigned) _start & ~(PAGE_SIZE - 1);

	/*
	 * REMAP the text segment as read-only, unless GDB is enabled.
	 */
	if (! enable_gdb) {
#ifdef  DEBUG_SVM
		printf("Mapping 0x%x to 0x%x\n",
		       (int) start_addr, (int)start_of_data);
#endif	
		if (svm_map_range((oskit_addr_t) start_addr,
				  (oskit_addr_t) kvtophys(start_addr),
				  (oskit_addr_t) start_of_data -
				  (oskit_addr_t) start_addr,
				  (PTE_MBITS_VALID|PTE_MBITS_R)))
			panic("Can't direct-map text memory");
	}

	/*
	 * REMAP the bottom page of the stack as read-only. 
	 */
#ifdef  DEBUG_SVM
	printf("Mapping 0x%x to 0x%x\n", (int) base_stack_start,
	       (int) base_stack_start + PAGE_SIZE);
#endif	
	if (svm_map_range((oskit_addr_t) base_stack_start,
			  (oskit_addr_t) kvtophys(base_stack_start), PAGE_SIZE,
			  PTE_MBITS_VALID|PTE_MBITS_R))
		panic("Can't direct-map stack redzone");

	/*
	 * Allocate a memory map for the virtual range. 
	 */
	amm_init(&svm_amm, 0x40000000, 0x80000000);

#if 1
	/*
	 * But fill in the physical memory portions so that we can
	 * associate protection values. Note, we have to be careful
	 * not to let these areas be deallocated, so mark them as
	 * AMM allocated and SVM reserved. 
	 *
	 * Since physical memory is discontiguous on the sharks,
	 * we do the best we can to record everything:
	 *
	 *	- record the statically loaded kernel memory
	 *	- walk through the malloc_lmm recording anything that
	 *	  is currently free
	 */
	{
		oskit_addr_t addr, lastaddr;
		oskit_size_t size;
		lmm_flags_t flags;

#ifdef  DEBUG_SVM
		printf("Adding physical memory to SVM AMM\n");
#endif
		addr = start_addr;
		lastaddr = round_page(end);
#ifdef  DEBUG_SVM
		printf("  kernel memory [%x - %x)...\n", addr, lastaddr);
#endif
		if (amm_modify(&svm_amm, addr, lastaddr - addr,
			       AMM_ALLOCATED|SVM_PROT_PHYS, 0))
			panic("amm_modify failed!\n");
		addr = lastaddr;
		lastaddr = phystokv(phys_mem_max);
		while (addr < lastaddr) {
			lmm_find_free(&malloc_lmm, &addr, &size, &flags);
			addr = trunc_page(addr);
			size = round_page(size);
#ifdef  DEBUG_SVM
			printf("  freemem [%x - %x)...\n", addr, addr+size);
#endif
			if (amm_modify(&svm_amm, addr, size,
				       AMM_ALLOCATED|SVM_PROT_PHYS, 0))
				panic("amm_modify failed!\n");
			addr = addr + size;
		}
	}
#endif
}

