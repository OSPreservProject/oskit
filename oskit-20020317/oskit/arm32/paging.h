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
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University.
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *	Definitions relating to arm32 page directories and page tables.
 */
#ifndef	_OSKIT_ARM32_PAGING_H_
#define _OSKIT_ARM32_PAGING_H_

#include <oskit/page.h>

#define PDESHIFT	20	/* page descriptor shift */
#define PDEMASK		0xfff	/* mask for page descriptor index */
#define PTESHIFT	12	/* page table shift */
#define PTEMASK		0x0ff	/* mask for page table index */

/*
 * A page table is 1024 bytes in size, and is aligned to its size.
 * A page directory (TTB) is 4 pages in size, and is aligned to its size.
 */
#define ARM32_PT_SHIFT		10
#define ARM32_PT_SIZE		1024
#define ARM32_PD_SHIFT		16
#define ARM32_PD_SIZE		0x4000

/*
 *	Convert linear offset to page descriptor/page table index
 */
#define va2pdenum(a)	(((a) >> PDESHIFT) & PDEMASK)
#define va2ptenum(a)	(((a) >> PTESHIFT) & PTEMASK)

/*
 *	Convert page descriptor/page table index to linear address
 */
#define pdenum2va(a)	((oskit_addr_t)(a) << PDESHIFT)
#define ptenum2va(a)	((oskit_addr_t)(a) << PTESHIFT)

/*
 *	Number of ptes/pdes in a page table/directory.
 */
#define NPTES	(ARM32_PT_SIZE/sizeof(pt_entry_t))
#define NPDES	(ARM32_PD_SIZE/sizeof(pt_entry_t))

/*
 * Hardware pde bit defintions (to be used directly on the pde entries
 *	without using the bit fields).
 */
#define ARM32_PDE_TYPE_INVALID	0x00000000
#define ARM32_PDE_TYPE_PAGETBL	0x00000001
#define ARM32_PDE_TYPE_SECTION	0x00000002
#define ARM32_PDE_TYPE_RESERVE	0x00000003
#define ARM32_PDE_TYPE_MASK     0x00000003
#define ARM32_PDE_BUFFERABLE    0x00000004
#define ARM32_PDE_CACHEABLE	0x00000008
#define ARM32_PDE_DOMAIN_MASK	0x000001E0
#define ARM32_PDE_AP_KR		0x00000000
#define ARM32_PDE_AP_KRW	0x00000400
#define ARM32_PDE_AP_KRWUR	0x00000800
#define ARM32_PDE_AP_KRWURW	0x00000C00
#define ARM32_PDE_AP_MASK	0x00000C00
#define ARM32_PDE_PFN_PAGETBL	0xFFFFFC00
#define ARM32_PDE_PFN_SECTION	0xFFF00000

/*
 *	Hardware pte bit definitions (to be used directly on the ptes
 *	without using the bit fields).
 */
#define ARM32_PTE_TYPE_INVALID	0x00000000
#define ARM32_PTE_TYPE_BPAGE	0x00000001
#define ARM32_PTE_TYPE_SPAGE	0x00000002
#define ARM32_PTE_TYPE_RESERVE	0x00000003
#define ARM32_PTE_TYPE_MASK	0x00000003
#define ARM32_PTE_BUFFERABLE    0x00000004
#define ARM32_PTE_CACHEABLE	0x00000008
#define ARM32_PTE_CONTROL_MASK  0x0000000F
#define ARM32_PTE_AP0		0x00000030
#define ARM32_PTE_AP1		0x000000C0
#define ARM32_PTE_AP2		0x00000300
#define ARM32_PTE_AP3		0x00000C00
#define ARM32_PTE_PFN_BPAGE     0xFFFF0000
#define ARM32_PTE_PFN_SPAGE     0xFFFFF000

/*
 *	Access permission bits by themselves.
 */
#define ARM32_AP_KR		0x0
#define ARM32_AP_KRW		0x1
#define ARM32_AP_KRWUR		0x2
#define ARM32_AP_KRWURW		0x3

/*
 *	Domain types
 */
#define ARM32_DOMAIN_FAULT	0x00
#define ARM32_DOMAIN_CLIENT	0x01
#define ARM32_DOMAIN_RESERVED	0x02
#define ARM32_DOMAIN_MANAGER	0x03

/*
 * By default, all 4 AP entries are set the same.
 */
#define PT_AP(x)	((x << 10) | (x << 8) | (x << 6)  | (x << 4))
#define PT_AP0(x)	(x << 4)
#define PT_AP1(x)	(x << 6)
#define PT_AP2(x)	(x << 8)
#define PT_AP3(x)	(x << 10)
#define PT_PTE2AP0(x)	((x >> 4) & 0x3)

/*
 *	Macros to translate between (small) page table entry values
 *	and physical addresses.
 */
#define	pa_to_pte(a)		((a) & ARM32_PTE_PFN_SPAGE)
#define	pte_to_pa(p)		((p) & ARM32_PTE_PFN_SPAGE)

#define	pa_to_pde(a)		((a) & ARM32_PDE_PFN_PAGETBL)
#define	pde_to_pa(p)		((p) & ARM32_PDE_PFN_PAGETBL)

/*
 *	Big Page macros. These are 64K pages.
 */
#define BIGPAGE_SHIFT		16
#define BIGPAGE_SIZE		(1 << BIGPAGE_SHIFT)
#define BIGPAGE_MASK		(BIGPAGE_SIZE - 1)

#define round_bigpage(x)	((oskit_addr_t)((((oskit_addr_t)(x))	\
				+ BIGPAGE_MASK) & ~BIGPAGE_MASK))
#define trunc_bigpage(x)	((oskit_addr_t)(((oskit_addr_t)(x))	\
				& ~BIGPAGE_MASK))

#define	bigpage_aligned(x)	((((oskit_addr_t)(x)) & BIGPAGE_MASK) == 0)

/*
 *	Superpage macros. These are 1MB pages.
 */
#define SUPERPAGE_SHIFT		20
#define SUPERPAGE_SIZE		(1 << SUPERPAGE_SHIFT)
#define SUPERPAGE_MASK		(SUPERPAGE_SIZE - 1)

#define round_superpage(x)	((oskit_addr_t)((((oskit_addr_t)(x))	\
				+ SUPERPAGE_MASK) & ~SUPERPAGE_MASK))
#define trunc_superpage(x)	((oskit_addr_t)(((oskit_addr_t)(x))	\
				& ~SUPERPAGE_MASK))

#define	superpage_aligned(x)	((((oskit_addr_t)(x)) & SUPERPAGE_MASK) == 0)

#ifndef ASSEMBLER
#include <oskit/compiler.h>
#include <oskit/arm32/types.h>
#include <oskit/arm32/proc_reg.h>

/*
 *	Page Table Entry
 */
typedef unsigned int	pt_entry_t;
#define PT_ENTRY_NULL	((pt_entry_t *) 0)

typedef unsigned int	pd_entry_t;
#define PD_ENTRY_NULL	((pt_entry_t *) 0)

/*
 * Write the page translation table base register (TTB). This register
 * *cannot* be read.
 *
 * BEWARE: Changing the TTB often requires flushing the TLB and the caches
 * since the ARM is virtually cached (*and* virtually tagged).
 */
OSKIT_INLINE void
set_ttb(oskit_addr_t pdir)
{
	unsigned int addr = (unsigned int) pdir;
	
	asm volatile ("mcr 15, 0, %0, c2, c0, 0" : : "r" (addr));
}

#define inval_tlb()	flush_tlb_ID()

#endif /* !ASSEMBLER */

#endif	_OSKIT_ARM32_PAGING_H_
