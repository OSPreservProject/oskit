/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * Definitions for the base VM environment provided by the OS toolkit:
 * how to change pointers between physical, virtual, and linear addresses, etc.
 */
#ifndef	_OSKIT_X86_BASE_VM_H_
#define _OSKIT_X86_BASE_VM_H_

#include <oskit/x86/types.h>


/* This variable is expected always to contain
   the kernel virtual address at which physical memory is mapped.
   It may change as paging is turned on or off.  */
extern oskit_addr_t phys_mem_va;


/* Calculate a kernel virtual address from a physical address.  */
#define phystokv(pa)	((oskit_addr_t)(pa) + phys_mem_va)

/* Same, but in reverse.
   This only works for the region of kernel virtual addresses
   that directly map physical addresses.  */
#define kvtophys(va)	((oskit_addr_t)(va) - phys_mem_va)


/* This variable contains the kernel virtual address
   corresponding to linear address 0.
   In the absence of paging,
   linear addresses are always the same as physical addresses.  */
extern oskit_addr_t linear_base_va;

/* Convert between linear and kernel virtual addresses.  */
#define lintokv(la)	((oskit_addr_t)(la) + linear_base_va)
#define kvtolin(va)	((oskit_addr_t)(va) - linear_base_va)


/* If the kernel was started from real mode,
   this variable contains our original real-mode code segment.  */
extern unsigned short real_cs;


#endif /* _OSKIT_X86_BASE_VM_H_ */
