/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * Functions for making real-mode BIOS or DOS calls on PCs
 * in the oskit's base environment.
 */
#ifndef _OSKIT_X86_PC_BASE_REAL_H_
#define _OSKIT_X86_PC_BASE_REAL_H_

struct trap_state;

/*
 * Vector to the 32-bit function to use to make a BIOS or DOS call
 * in real/v86 mode through a software interrupt.
 * The trapno field in the trap_state contains the interrupt number.
 * Only the general registers, eflags, v86_ds, and v86_es registers
 * in the trap state are used and modified by this routine.
 * Always be sure to initialize the eflags register (usually to zero),
 * to avoid strange processor settings during the real-mode call.
 */
extern void (*base_real_int)(struct trap_state *state);

/*
 * This function finds all the real (below 640K) memory in the system
 * using the appropriate BIOS call and adds it to the malloc_lmm pool,
 * with the exception of the memory containing our own kernel image,
 * if it is located in real memory.
 */
void base_conv_mem_init(void);

/*
 * This function finds all extended memory in the system
 * using the appropriate BIOS call and adds it to the malloc_lmm pool,
 * with the exception of the memory containing our own kernel image,
 * if it is located in extended memory.
 */
void base_ext_mem_init(void);

#endif /* _OSKIT_X86_PC_BASE_REAL_H_ */
