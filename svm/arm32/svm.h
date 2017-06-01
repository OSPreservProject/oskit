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
 * Private header file for SVM. This is the arm32 specific stuff.
 */
#ifndef _SVM_ARM32_INT_H_
#define _SVM_ARM32_INT_H__

#include <oskit/arm32/page.h>
#include <oskit/arm32/trap.h>
#include <oskit/arm32/proc_reg.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/base_paging.h>
#include <oskit/arm32/base_stack.h>
#include <oskit/lmm.h>

extern struct lmm malloc_lmm;

/*
 * Internal machine dependent prottypes.
 */
int		sdir_map_page(oskit_addr_t va, unsigned int mapping_bits);

/*
 * There are no unused bits in the PTE. Even worse, the hardware does not
 * implement mod/ref bits, so these have to be done in software too. The
 * bits below are kept in a parallel page directory, although it is a
 * field of bytes. Operations must split/combine the actual PTE information
 * and the software bits. Note that these bits must fit in a byte, and that
 * they are shifted down. See below.
 */
#define ARM32_PTE_ZFILL	0x00008000
#define ARM32_PTE_PAGED	0x00004000
#define ARM32_PTE_WIRED 0x00002000
#define ARM32_PTE_REF	0x00001000
#define ARM32_PTE_MOD   0x00000800

/*
 * Inline function to flush a page from the TLB.
 */
OSKIT_INLINE void ftlbentry(oskit_addr_t la)
{
	flush_tlb_ID();	
}

/*
 * Define machine independent names for mode bits. In general, most machines
 * will enable such a mapping since MMUs tend to be similar.
 *
 * Note we choose the supervisor bits since that gives us read-only pages.
 */
#define PTE_MBITS_VALID		ARM32_PTE_TYPE_SPAGE
#define PTE_MBITS_R		PT_AP0(ARM32_AP_KR)
#define PTE_MBITS_RW		PT_AP0(ARM32_AP_KRW)
#define PTE_MBITS_REF		ARM32_PTE_REF
#define PTE_MBITS_MOD		ARM32_PTE_MOD
#define PTE_MBITS_ZFILL		ARM32_PTE_ZFILL
#define PTE_MBITS_PAGED		ARM32_PTE_PAGED
#define PTE_MBITS_WIRED		ARM32_PTE_WIRED
#define PTE_MBITS_MBITS		(~ARM32_PTE_PFN_SPAGE)

/*
 * Combine address and mode bits into a PTE.
 */
#define create_pte(a, m)	((pt_entry_t) (((oskit_addr_t) (a)) | (m)))

/*
 * Extract mode bits from a pte.
 */
#define	pte_to_modebits(p)	((p) & ~ARM32_PTE_PFN_SPAGE)

/*
 * The parallel page directory. Note that this will be a virtual address,
 * not a physical address like base_pdir_pa.
 */
extern unsigned char		**svm_sdir;

/*
 * The software bits. A software directory entry points to an array of
 * characters. 
 */
typedef unsigned char	       *st_table_t;
typedef unsigned char		st_entry_t;

/*
 * Convert between the software bits entry and the software bits.
 */
#define softbits_to_ste(s)	((s >> 8) & 0xFF)
#define ste_to_softbits(s)	(s << 8)

/*
 * Find the software bits table given a va.
 */
#define sdir_get_stab(va)					\
	(svm_sdir[va2pdenum(va)])
#define sdir_set_stab(va, stab)					\
	(svm_sdir[va2pdenum(va)] = (stab))

/*
 * Find the software bits entry (return a pointer to it) given a va.
 */
#define stab_find_ste(stab, va)					\
	(&((stab)[va2ptenum(va)]))

/*
 * Get the software bits given a va.
 */
OSKIT_INLINE unsigned int
sdir_get_ste(oskit_addr_t va)
{
	st_table_t	stab;
	st_entry_t	*ste;

	stab = sdir_get_stab(va);
	ste  = stab_find_ste(stab, va);

	return *ste;
}
#endif
