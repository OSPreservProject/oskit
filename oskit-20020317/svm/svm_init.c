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
 * Notes:
 *
 * What about locking and other issues of execution environments.
 *
 * I'm not using superpages at all. That complicates things 'cause anytime
 * an application wants to modify a page in a superpage, the entire superpage
 * has to be broken up into invidual pages. Too nasty for a first draft.
 *
 * Unused memory cannot be protected since the lmm requires write
 * access to the pages to maintain the free lists. Besides, there is
 * no telling what memory the kernel will access. This means the applicaton
 * can write anything it wants to, except non-existent memory.
 *
 * I am allowing just READ and READ/WRITE prots.  PROT_NONE is not
 * allowed.
 *
 * Should morecore be redefined so that memory shortages in the malloc pool
 * are handled properly?
 *
 * Pages are read and written to/from the swap area through the virtual
 * address.
 */

/*
 * Things to possibly do:
 *
 * 1) External pager interface. Add export of policy for selecting pages
 * to aid in discardable page.
 *
 * 2) Provide support for specifying discardable pages (SML paper) and
 * a way to flush disardable pages from the system.
 *
 * 3) In the threads world, use a thread for a simple pageout daemon.
 */

#include "svm_internal.h"

/*
 * Use the AMM to track virtual mappings.
 */
amm_t		svm_amm;

/*
 * Application level SEGV handler for page faults.
 */
int		(*svm_segv_handler)(oskit_addr_t la, int rw);

/*
 * Enabled?
 */
int		svm_enabled = 0;

/*
 * Thread safe locking.
 */
oskit_lock_t	*svm_lock = 0;

#ifdef KNIT
extern oskit_lock_mgr_t *oskit_lock_mgr;
#define lock_mgr oskit_lock_mgr
#endif

/*
 * The VM region starts after the end of physical memory, and extends
 * as far as possible.
 */
void
svm_init(oskit_absio_t *pager_absio)
{
#ifndef KNIT
	oskit_lock_mgr_t	*lock_mgr;
#endif

	if (svm_enabled)
		return;

	/*
	 * Machine dependent initialization.
	 */
	svm_machdep_init();

	/*
	 * This turns on paging!
	 */
	base_paging_load();

	/*
	 * Initialize the stuff for catching stack overflow.
	 */
	svm_redzone_init();

	/*
	 * See if thread-safe locks are required.
	 */
#ifndef KNIT
	if (oskit_lookup_first(&oskit_lock_mgr_iid, (void *) &lock_mgr))
		panic("svm_init: oskit_lookup_first");
#endif

	if (lock_mgr) {
		if (oskit_lock_mgr_allocate_lock(lock_mgr, &svm_lock))
			panic("svm_init: oskit_lock_mgr_allocate_lock");
	}

	/*
	 * XXX: Must turn this on before starting the pager. See mem.c
	 */
	svm_enabled = 1;

	/*
	 * Initialize the pager if requested.
	 */
	if (pager_absio) {
		if (svm_pager_init(pager_absio))
			panic("svm_init: Could init the pager!\n");
	}

	/*
	 * Hook in a new page fault handler.
	 */
	base_trap_handlers[T_PAGE_FAULT] = svm_page_fault_handler;

	/*
	 * and initialize the application level handler to null so
	 * that the default is to send a SIGSEGV to the application.
	 */
	svm_segv_handler = 0;
}

void
svm_shutdown()
{
	extern int	svm_pagein_count, svm_pageout_count;
	extern int	svm_pagein_time, svm_pageout_time;
	extern int	svm_swapread_time, svm_swapwrite_time;
	extern int	svm_swapwrite_count, svm_swapread_count;

	printf("svm_shutdown: swapread  count: %d pages. time %d\n"
	       "              swapwrite count: %d pages. time %d\n",
	       svm_swapread_count, svm_swapread_time,
	       svm_swapwrite_count, svm_swapwrite_time);

	printf("svm_shutdown: pagein  count: %d pages. time %d\n"
	       "              pageout count: %d pages. time %d\n",
	       svm_pagein_count, svm_pagein_time,
	       svm_pageout_count, svm_pageout_time);
}
