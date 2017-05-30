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
/*
 *	Basic page directory and page table management routines for the x86.
 */
#ifndef	_OSKIT_X86_BASE_PAGING_H_
#define _OSKIT_X86_BASE_PAGING_H_

#include <oskit/compiler.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/paging.h>
#include <oskit/x86/base_vm.h>

/*
 * Find the entry in a page directory or page table
 * for a particular linear address.
 */
#define pdir_find_pde(pdir_pa, la)					\
	(&((pd_entry_t*)phystokv(pdir_pa))[lin2pdenum(la)])
#define ptab_find_pte(ptab_pa, la)					\
	(&((pt_entry_t*)phystokv(ptab_pa))[lin2ptenum(la)])

OSKIT_BEGIN_DECLS

/*
 * Find a page table entry given a page directory and a linear address.
 * Returns NULL if there is no page table covering that address.
 * Assumes that if there is a valid PDE, it's a page table, _not_ a 4MB page.
 */
OSKIT_INLINE pt_entry_t *pdir_find_pte(oskit_addr_t pdir_pa, oskit_addr_t la)
{
	pd_entry_t *pde = pdir_find_pde(pdir_pa, la);
	if (!(*pde & INTEL_PDE_VALID))
		return 0;
	return ptab_find_pte(pde_to_pa(*pde), la);
}

/*
 * Get the value of a page table entry,
 * or return zero if no such entry exists.
 * As above, doesn't check for 4MB pages.
 */
OSKIT_INLINE pt_entry_t pdir_get_pte(oskit_addr_t pdir_pa, oskit_addr_t la)
{
	pt_entry_t *pte = pdir_find_pte(pdir_pa, la);
	return pte ? *pte : 0;
}

/*
 * Functions called by the following routines to allocate a page table.
 * Supplies a single page of CLEARED memory, by its physical address.
 * Returns zero if successful, nonzero on failure.
 * The default implementation simply allocates a page from the malloc_lmm,
 * returning -1 if the LMM runs out of memory.
 * This implementation can be overridden, of course.
 */
int ptab_alloc(oskit_addr_t *out_ptab_pa);

/*
 * A Page Directory looks just like a Page Table, on this machine!
 */
#define pdir_alloc(out_pa)	ptab_alloc(out_pa)
#define pdir_free(ptab_pa)	ptab_free(ptab_pa)

/*
 * Free a page table allocated using ptab_alloc().
 * The caller must ensure that it is not still in use in any page directories.
 */
void ptab_free(oskit_addr_t ptab_pa);


/*
 * Map a 4KB page into a page directory.
 * Calls ptab_alloc if a new page table needs to be allocated.
 * If ptab_alloc returns nonzero, pdir_map_page aborts with that value.
 * Otherwise, inserts the mapping and returns zero.
 * Doesn't check for 4MB pages.
 */
int pdir_map_page(oskit_addr_t pdir_pa, oskit_addr_t la, pt_entry_t mapping);

/*
 * Unmap a 4KB page from a page directory.
 * Invalidates the page table entry for the page containing the indicated
 * virtual address.  May call ptab_free to release page table resources.
 * Doesn't check for 4MB pages.
 */
void pdir_unmap_page(oskit_addr_t pdir_pa, oskit_addr_t la);


/*
 * Map a continuous range of virtual addresses
 * to a continuous range of physical addresses.
 * If the processor supports 4MB pages, uses them if possible.
 * Assumes that there are no valid mappings already in the specified range.
 * The 'mapping_bits' parameter must include INTEL_PTE_VALID,
 * and may include other permissions as desired.
 */
int pdir_map_range(oskit_addr_t pdir_pa, oskit_addr_t la, oskit_addr_t pa,
		   oskit_size_t size, pt_entry_t mapping_bits);

/*
 * Change the permissions on an existing mapping range.
 * The 'new_mapping_bits' parameter must include INTEL_PTE_VALID,
 * and may include other permissions as desired.
 * Assumes that the mappings being changed were produced
 * by a previous call to pdir_map_range()
 * with identical linear address and size parameters.
 */
void pdir_prot_range(oskit_addr_t pdir_pa, oskit_addr_t la, oskit_size_t size,
		     pt_entry_t new_mapping_bits);

/*
 * Unmap a continuous range of virtual addresses.
 * Assumes that the mappings being unmapped were produced
 * by a previous call to pdir_map_range()
 * with identical linear address and size parameters.
 */
void pdir_unmap_range(oskit_addr_t pdir_pa, oskit_addr_t la, oskit_size_t size);

/*
 * Frees any unused page table pages associated with the indicated
 * page-aligned range in the given PDIR; the size is rounded up to whole pages.
 * Unused pages are those containing only invalid PTEs (resulting from
 * calls to unmap_page and unmap_range).  Used to reclaim page table resources.
 */
void pdir_clean_range(oskit_addr_t pdir_pa, oskit_addr_t la, oskit_size_t size);

/*
 * These functions are primarily for debugging VM-related code:
 * they pretty-print a dump of a page directory or page table
 * and all their mappings.
 */
void pdir_dump(oskit_addr_t pdir_pa);
void ptab_dump(oskit_addr_t ptab_pa, oskit_addr_t base_la);

/*
 * Initialize a basic paged environment.
 * Sets up the base_pdir (see below) which direct maps
 * all physical memory from 0 to (at least) phys_mem_max.
 * The client can then modify this page directory or create others as needed.
 * The base physical memory mappings are accessible from user mode by default.
 */
void base_paging_init(void);

/*
 * Enable the basic paged environment on the current processor
 * using the base_pdir_pa.
 * Also enables superpages if appropriate.
 */
void base_paging_load(void);

/*
 * Physical address of the base page directory,
 * set up by base_paging_init().
 */
extern oskit_addr_t base_pdir_pa;

OSKIT_END_DECLS

#endif	/* _OSKIT_X86_BASE_PAGING_H_ */
