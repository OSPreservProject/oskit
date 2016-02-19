/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
 * Code to enable the basic paged memory environment on a SHARK. Much of
 * this stuff would also need to be done when changing the TTB pointer.
 */

#include <oskit/debug.h>
#include <oskit/arm32/proc_reg.h>
#include <oskit/arm32/base_paging.h>
#include <oskit/arm32/base_vm.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/trap.h>

pd_entry_t *mypde;

void base_paging_load(void)
{
	int	i;
	
	/* What about FIQs? */
	disable_interrupts();

	/*
	 * Well, this is a major pain in the ass. I have not figured out
	 * how to clean/flush individual lines from the cache, so changes
	 * to the page directory and tables made through the virtual address,
	 * do not get flushed to main memory where the TLB walker scans them.
	 * So, go through and change all the page tables describing the
	 * page directory and page tables (:-)) to non cacheable/bufferable
	 * so that there is no cache coherence problem. Note that the pdir
	 * functions arrange for newly allocated page tables to have the
	 * proper bits changed.
	 *
	 * First do the page directory itself.
	 */
	pdir_prot_range(base_pdir_pa,
			phystokv(base_pdir_pa), ARM32_PD_SIZE,
			PT_AP(ARM32_AP_KRWURW) | ARM32_PTE_TYPE_SPAGE);

	/*
	 * Now go through the page directory, and for each valid page
	 * table, changes it bits.
	 */
	for (i = 0; i < ARM32_PD_SIZE / sizeof(pd_entry_t); i++) {
		mypde = (&((pd_entry_t*)phystokv(base_pdir_pa))[i]);

		if (! *mypde)
			continue;

		pdir_prot_range(base_pdir_pa,
				phystokv(pde_to_pa(*mypde)), PAGE_SIZE,
				PT_AP(ARM32_AP_KRWURW) | ARM32_PTE_TYPE_SPAGE);
	}

	/*
	 * According to the SA110 spec, the only way to properly clean/flush
	 * the data cache is to read from a special location in I/O space
	 * in a loop that causes the entire D cache to be cleaned before
	 * the write buffers are drained. Totally silly.
	 */
	arm32_cache_clean();

	flush_cache_I();
	drain_write_buffer();

	/* Set the translation table base register */
	set_ttb(base_pdir_pa);

	/* Flush the I+D TLBs! */
	flush_tlb_ID();

	/* Flush the I cache */
	flush_cache_I();

	/* Turn on the S bit so we get read-only pages! Must turn off R bit. */
	set_cpuctrl(get_cpuctrl() & ~CPU_CTRL_R);
	set_cpuctrl(get_cpuctrl() |  CPU_CTRL_S);

	/* Set the domain control register to do access permission checks. */
	set_domctrl(ARM32_DOMAIN_CLIENT);

	/* Turn on the M bit (MMU enable) */
	set_cpuctrl(get_cpuctrl() | CPU_CTRL_M);

	enable_interrupts();
}
