/*
 * Copyright (c) 1998, 1999 University of Utah and the Flux Group.
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
 * Task switch code.
 */
#include <oskit/config.h>
#include <oskit/arm32/asm.h>
#include <oskit/arm32/paging.h>
#include <oskit/arm32/softfloat.h>
#include <threads/pthread_internal.h>
#include <threads/arm32/pcb.h>

/*
 * r0 - next thread pointer.
 * r1 - lock pointer. Ignored.
 * r2 - current thread pointer.
 */
ENTRY(thread_switch_realmode)
	/*
	 * Save current context
	 */
	ldr	r3, [r2, #THREAD_PPCB]
	add	r3, r3, #PCB_R4

	/* Store integer registers */
        stmia	r3, {r4-r14}
	
#ifndef SOFTFLOAT
#error	"Need floating point support in thread switch!"
#endif

	/*
	 * Save is done, switch to new thread.
	 */
	ldr	r3, [r0, #THREAD_PPCB]
	add	r3, r3, #PCB_R4

       	/* Restore integer registers */
        ldmia	r3, {r4-r14}
	
#ifndef SOFTFLOAT
#error	"Need floating point support in thread switch!"
#endif
	/*
	 * Return to new thread's saved link register.
	 *
	 * XXX:	The return value is the new thread. We depend on this
	 * little artifact for passing the thread pointer to the start 
	 * routine for a new thread!
	 */
	mov	r15, r14
