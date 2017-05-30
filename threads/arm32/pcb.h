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
#ifndef _THREADS_ARM32_PCB_H_
#define _THREADS_ARM32_PCB_H_

#ifndef ASSEMBLER

#include <oskit/machine/types.h>

/*
 * Machine dependent PCB structure for threads.
 */
typedef struct {
	int		flags;
	oskit_u32_t	r0;
	oskit_u32_t	r1;
	oskit_u32_t	r2;
	oskit_u32_t	r3;
	oskit_u32_t	r4;
	oskit_u32_t	r5;
	oskit_u32_t	r6;
	oskit_u32_t	r7;
	oskit_u32_t	r8;
	oskit_u32_t	r9;
	oskit_u32_t	r10;
	oskit_u32_t	r11;	/* fp */
	oskit_u32_t	r12;	/* ip */
	oskit_u32_t	r13;	/* sp */
	oskit_u32_t	r14;	/* lr */
	oskit_u32_t	fstate[64];	/* Black Box */
} pcb_t;

#endif /* ASSEMBLER */

/*
 * Define constants for assembly code.
 */
#define PCB_R4		20

#endif /* _THREADS_ARM32_PCB_H_ */
