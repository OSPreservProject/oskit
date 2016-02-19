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

#include <svm/svm_internal.h>

extern int	enable_gdb;	/* XXX */

void
svm_machdep_init(void)
{
	extern char		_start[], end[], start_of_data[];
	oskit_addr_t		addr, start_addr;

	if (ptab_alloc(&base_pdir_pa))
		panic("Can't allocate kernel page directory");

	start_addr = (unsigned) _start & ~(PAGE_SIZE - 1);
 
	/*
	 * Map lower part of memory (ROM and I/O).
	 */
#ifdef  DEBUG_SVM
	printf("Mapping 0x%x to 0x%x\n", 0, (int) start_addr);
#endif
	if (svm_map_range(0, 0, (oskit_addr_t) start_addr,
			  PTE_MBITS_VALID|PTE_MBITS_RW))
		panic("Can't direct-map lower physical memory");

	/*
	 * Map the text segment as read-only.
	 */
#ifdef  DEBUG_SVM
	printf("Mapping 0x%x to 0x%x\n", (int) start_addr, (int)start_of_data);
#endif	
	if (svm_map_range((oskit_addr_t) start_addr,
		(oskit_addr_t) start_addr,
		(oskit_addr_t) start_of_data - (oskit_addr_t) start_addr,
		(enable_gdb ? (PTE_MBITS_VALID|PTE_MBITS_RW) :
		              (PTE_MBITS_VALID|PTE_MBITS_R))))
		panic("Can't direct-map text memory");

	/*
	 * Map data segment as read/write.
	 */
#ifdef  DEBUG_SVM
	printf("Mapping 0x%x to 0x%x\n", (int) start_of_data, round_page(end));
#endif
	if (svm_map_range((oskit_addr_t) start_of_data,
		(oskit_addr_t) start_of_data,
	        (oskit_addr_t) round_page(end) - (oskit_addr_t) start_of_data,
		PTE_MBITS_VALID|PTE_MBITS_RW))
		panic("Can't direct-map data memory");

	/*
	 * Map the bottom page of the stack as read-only. It was part of
	 * the data segment, so must unmap it first.
	 */
	svm_unmap_range((oskit_addr_t) base_stack_start, PAGE_SIZE);  

#ifdef  DEBUG_SVM
	printf("Mapping 0x%x to 0x%x\n", (int) base_stack_start,
	       (int) base_stack_start + PAGE_SIZE);
#endif	
	if (svm_map_range((oskit_addr_t) base_stack_start,
		(oskit_addr_t) base_stack_start, PAGE_SIZE,
		PTE_MBITS_VALID|PTE_MBITS_R))
		panic("Can't direct-map stack redzone");

	/*
	 * Map the rest of phys memory as so that the lmm works.
	 *
	 * XXX Do this in a page loop to avoid superpage creation.
	 */
#ifdef  DEBUG_SVM
	printf("Mapping 0x%x to 0x%x\n", round_page(end),
	       round_page(phys_mem_max));
#endif
	addr = (oskit_addr_t) round_page(end);
	while (addr < round_page(phys_mem_max)) {
		if (svm_map_range(addr, addr, PAGE_SIZE,
			PTE_MBITS_VALID|PTE_MBITS_RW))
			panic("Can't direct-map rest of physical memory");

		addr += PAGE_SIZE;
	}

	/*
	 * Set the WP bit in CR0. This bit turns on enforcement of read-only
	 * access when a user-mode page is accessed in supervisor mode.
	 */
	set_cr0(get_cr0() | CR0_WP);

	/*
	 * Allocate a memory map for the virtual range.
	 */
	amm_init(&svm_amm, round_page(phys_mem_max), AMM_MAXADDR);

	/*
	 * But fill in the physical memory portions so that we can
	 * associate protection values. Note, we have to be careful
	 * not to let these areas be deallocated, so mark them as
	 * AMM allocated and SVM reserved. 
	 */
	if (amm_modify(&svm_amm, start_addr,
		       round_page(phys_mem_max) - start_addr,
		       AMM_ALLOCATED|SVM_PROT_PHYS, 0))
		panic("amm_modify failed!\n");
}

