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
 * Private header file for SVM. This is the x86 specific stuff.
 */
#ifndef _SVM_X86_INT_H_
#define _SVM_X86_INT_H__

#include <oskit/x86/page.h>
#include <oskit/x86/trap.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/base_paging.h>
#include <oskit/x86/base_stack.h>
#include <oskit/x86/pc/phys_lmm.h>

/*
 * Use some of the undefined bits in the PTE.
 */
#define INTEL_PTE_ZFILL	0x00000800
#define INTEL_PTE_PAGED	0x00000400
#define INTEL_PTE_WIRED 0x00000200

/*
 * Inline function to flush a page from the TLB.
 */
OSKIT_INLINE void ftlbentry(oskit_addr_t la)
{
	asm volatile("invlpg (%0)" : : "r" (la) : "memory");
}

/*
 * Define machine independent names for mode bits. In general, most machines
 * will enable such a mapping since MMUs tend to be similar.
 *
 * Note that for R/W bits, we chose the USER mode bits, since thats the
 * the mode in which we get read-only pages.
 */
#define PTE_MBITS_VALID		INTEL_PTE_VALID
#define PTE_MBITS_R		INTEL_PTE_USER
#define PTE_MBITS_RW		(INTEL_PTE_WRITE|INTEL_PTE_USER)
#define PTE_MBITS_REF		INTEL_PTE_REF
#define PTE_MBITS_MOD		INTEL_PTE_MOD
#define PTE_MBITS_ZFILL		INTEL_PTE_ZFILL
#define PTE_MBITS_PAGED		INTEL_PTE_PAGED
#define PTE_MBITS_WIRED		INTEL_PTE_WIRED
#define PTE_MBITS_MBITS		(~INTEL_PTE_PFN)

/*
 * Combine address and mode bits into a PTE.
 */
#define create_pte(a, m)	((pt_entry_t) (((oskit_addr_t) (a)) | (m)))

/*
 * Extract mode bits from a pte.
 */
#define	pte_to_modebits(p)	((p) & ~INTEL_PTE_PFN)

#undef  get_tsc
#define get_tsc() \
    ({ \
        unsigned long low, high; \
        asm volatile( \
        ".byte 0x0f; .byte 0x31" \
        : "=d" (high), "=a" (low)); \
        low; \
    })
#endif
