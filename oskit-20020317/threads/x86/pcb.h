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
#ifndef _X86_PCB_H_
#define _X86_PCB_H_

#ifndef ASSEMBLER

#include <oskit/machine/types.h>

/*
 * Machine dependent PCB structure for threads.
 */
typedef struct {
	int		flags;
	oskit_u32_t	eip;
	oskit_u32_t	ebx;
	oskit_u32_t	esp;
	oskit_u32_t	ebp;
	oskit_u32_t	esi;
	oskit_u32_t	edi;
	oskit_u32_t	cr2;
	oskit_u32_t	fstate[32];	/* Black Box */
} pcb_t;

#endif /* ASSEMBLER */

/*
 * Define constants for assembly code.
 */
#define PCB_EIP		4
#define PCB_EBX		8
#define PCB_ESP		12
#define PCB_EBP		16
#define PCB_ESI		20
#define PCB_EDI		24
#define PCB_CR2		28
#define PCB_FSTATE	32

#endif /* _X86_PCB_H_ */
