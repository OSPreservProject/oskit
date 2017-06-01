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

/*
 * Private header file for SVM
 */
#ifndef _SVM_INT_H_
#define _SVM_INT_H__

#include <oskit/svm.h>

#include <stdio.h>
#include <oskit/amm.h>
#include <oskit/error.h>
#include <oskit/debug.h>
#include <oskit/com/services.h>
#include <oskit/com/lock_mgr.h>
#include <oskit/com/lock.h>
#include <oskit/io/absio.h>
#include <oskit/machine/physmem.h>
#include "machine/svm.h"

/* #define DEBUG_SVM		WhyNot */
/* #define DEBUG_SVM_PAGEOUT	WhyNot */

extern amm_t		svm_amm;
extern int		svm_enabled;
extern int		svm_paging;

/*
 * Thread-safe locking support. Very simple; use a single lock to
 * protect the enture SVM system since there is no per-thread state,
 * only global state.
 */
extern oskit_lock_t	*svm_lock;
#define SVM_LOCK()	if (svm_lock) oskit_lock_lock(svm_lock)
#define SVM_UNLOCK()	if (svm_lock) oskit_lock_unlock(svm_lock)

/*
 * Application level SEGV handler for page faults. The return values
 * are specified in the external svm.h header.
 */
extern int (*svm_segv_handler)(oskit_addr_t la, int rw);

/*
 * This flag value is or'ed in with the protection for physical memory
 * regions that are under the control of the LMM. This allows us to
 * associate protections with them, but not deallocate them.
 */
#define SVM_PROT_PHYS	SVM_PROT_RESVD

/*
 * Internal trap handler.
 */
extern int svm_page_fault_handler(struct trap_state *ts);

#define ptob(x)		((x) << PAGE_SHIFT)
#define btop(x)		((x) >> PAGE_SHIFT)

/*
 * Internal prototypes
 */
int		svm_fault(oskit_addr_t vaddr, int eflags);
void		svm_pageout(void);
void		svm_pageout_page(oskit_addr_t vaddt);
void		svm_pagein_page(oskit_addr_t vaddt);
void		svm_redzone_init(void);
int		svm_pager_init(oskit_absio_t *pager_absio);
int		svm_swapread(int diskpage, oskit_addr_t buf);
int		svm_swapwrite(int diskpage, oskit_addr_t buf);
int		svm_getdiskpage(void);
void		svm_freediskpage(int diskpage);
oskit_addr_t	svm_alloc_physpage(void);
void		svm_dealloc_physpage(oskit_addr_t paddr);
oskit_size_t	svm_physmem_avail(void);

/*
 * These must be defined in machine dependent code.
 */
int		svm_map_range(oskit_addr_t va, oskit_addr_t pa,
			oskit_size_t size, unsigned int mapping_bits);
int		svm_unmap_range(oskit_addr_t va, oskit_size_t size);
int		svm_find_mapping(oskit_addr_t va, oskit_addr_t *pa,
			unsigned int *mbits);
int		svm_prot_range(oskit_addr_t va, oskit_size_t len,
			unsigned int prot);
int		svm_change_mapping(oskit_addr_t va, oskit_addr_t pa,
			unsigned int mbits);
void		svm_machdep_init(void);
int		svm_mem_map_phys(oskit_addr_t pa, oskit_size_t size,
			void **addr, int flags);

/*
 * Ptov table.
 */
extern oskit_addr_t	*svm_ptov;

#define SVM_NULL_PTOV	((oskit_addr_t) -1)

#define SVM_PTOV(paddr) \
	(svm_ptov[btop((oskit_addr_t) paddr - phys_mem_min)])
#endif
