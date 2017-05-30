/*
 * Copyright (c) 1994-1996 Sleepless Software
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Remote PC serial-line debugging for the Flux OS Toolkit
 */

#include <oskit/arm32/asm.h>

	.text

/*
 * Copy between kernel address space and a GDB buffer,
 * detecting and recovering from any invalid accesses that occur.
 * Since we are merely copying in the same address space (the kernel),
 * both the gdb_copyin and gdb_copyout routines can be the same.
 *
 * arg0:	source address
 * arg1:	destination address
 * arg2:	byte count
 */
ENTRY(gdb_copy)
	ldr	r3, Lgdb_trap_recover
	ldr	ip, copy_fail
	str	ip, [r3]
	
	sub	r2, r2, #1
	cmn	r2, #1
	moveq	pc, lr
Loop:	
	sub	r2, r2, #1
	ldrb	r3, [r0], #1
	cmn	r2, #1
	strb	r3, [r1], #1
	bne	Loop

	mov	r0, #0			/* return 0 for success */
	
copy_ret:
	ldr	r3, Lgdb_trap_recover
	mov	ip, #0
	str	ip, [r3]
	mov	pc, lr

copy_fail:
	mov	r0, #-1			/* return -1 for failure */
	b	copy_ret		/* pop frame and return */

Lgdb_trap_recover:
	.word	gdb_trap_recover
