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

#include <stdio.h>
#include <oskit/arm32/trap.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/proc_reg.h>

void trap_dump(const struct trap_state *st)
{
	printf("Dump of trap_state at %p (trapno: %d):\n", st, st->trapno);
	printf("R0  0x%08x R1  0x%08x R2  0x%08x R3  0x%08x\n",
		st->r0, st->r1, st->r2, st->r3);
	printf("R4  0x%08x R5  0x%08x R6  0x%08x R7  0x%08x\n",
		st->r4, st->r5, st->r6, st->r7);
	printf("R8  0x%08x R9  0x%08x R10 0x%08x R11 0x%08x R12 0x%08x\n",
		st->r8, st->r9, st->r10, st->r11, st->r12);
	printf("USP 0x%08x ULR 0x%08x KSP 0x%08x KLR 0x%08x\n",
		st->usr_sp, st->usr_lr, st->svc_sp, st->svc_lr);
	printf("PC  0x%08x SPSR 0x%08x\n",
		st->pc, st->spsr);
	printf("FAR 0x%08x FSR 0x%08x\n",
	       get_far(), get_fsr());
}
