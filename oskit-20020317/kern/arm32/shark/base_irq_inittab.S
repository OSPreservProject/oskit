/*
 * Copyright (c) 1999, 2000 University of Utah and the Flux Group.
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
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Interrupt handler. This operates by looping through the request register
 * for each of the master and slaves (two loops). According to the NetBSD
 * documentation, ISA bus access is terribly slow, and NetBSD goes to great
 * pains to avoid it. I use a much simpler and slower approach, but this is
 * intended to be a prototype ...	 
 */

#include <oskit/arm32/asm.h>
#include <oskit/arm32/base_trap.h>

#define IO_ICU1		0x20
#define IO_ICU2		0xa0
#define IO_ICU1_OCW	0x21
#define IO_ICU2_OCW	0xa1
#define	ICU_EOI		0x20
#define SLAVE_ON_IR2	0x04

/*
 * r4  - irq counter for master and slave loop
 * r5  - slave imask
 * r6  - master imask
 * r7  - isa_iobus_address pointer
 * r8  - master iir
 * r9  - slave iir
 * r10 - trap state pointer
 */
	.text

ENTRY(base_irq_trap_handler)
	stmfd	sp!, {lr}
	mov	r10, r0
	
	/*
	 * Adjust the PC in the trap frame for an IRQ.
	 */
	ldr	r1, [r0, #(TRAP_STATE_PC)]
	sub	r1, r1, #0x00000004
	str     r1, [r0, #(TRAP_STATE_PC)]

	/*
	 * Load the address of the ISA I/O bus, then indirect to read 8259
	 * interrupt request registers.
	 */
	ldr	r7, Lisa_iobus_address
	ldr	r7, [r7]
	ldrb	r8, [r7, #IO_ICU1]        /* ocw3 = irr */
	ldrb	r9, [r7, #IO_ICU2]        /* ocw3 = irr */
	bic	r8, r8, #SLAVE_ON_IR2	  /* always clear IRQ 2 (slave 8259) */
	
	/* 
	 * Increment the hardware interrupt nesting counter
	 */
	ldr	r2, Lbase_irq_nest
	ldr	r3, [r2]
	add	r3, r3, #0x1
	str	r3, [r2]

	cmp	r8, #0
	beq	skipmpic

	/* Save the current master PIC mask */
	ldrb	r6, [r7, #IO_ICU1_OCW]

	/* Remove masked bits from the master iir.  */
	bic	r8, r8, r6
	cmp	r8, #0
	beq	skipmpic

	/* Mask out the interrupting IRQs on the master */
	orr	r3, r6, r8
	strb    r3, [r7, #IO_ICU1_OCW]

skipmpic:
	cmp	r9, #0
	beq	skipspic
	
	/* Save the current slave PIC mask */
	ldrb	r5, [r7, #IO_ICU2_OCW]

	/* Remove masked bits from the slave iir */
	bic	r9, r9, r5
	cmp	r9, #0
	beq	skipspic
	
	/* Mask out the interrupting IRQs on the slave */
	orr	r3, r5, r9
	strb    r3, [r7, #IO_ICU2_OCW]

skipspic:
	/*
	 * Now loop through and call the handler for each pending interrupt.
	 */
	cmp	r8, #0
	beq	mskip
	
        mov	r4, #0
mloop:
	mov	r3, #1
	mov	r3, r3, lsl r4		/* Build the 1 bit mask */
	tst	r8, r3			/* Is a bit set? */
	beq	mnext			/* No, then loop */

	/*
	 * Call the interrupt handler. Be sure to stash the irq number
	 * in the trap frame, and pass the stack pointer as arg0.
	 */
	str     r4, [r10, #(TRAP_STATE_INTNO)]
	ldr	r3, Lbase_irq_handlers
	ldr	r3, [r3, r4, asl #2]	/* r3 = base_irq_handlers[irq] */
	mov	r0, r10
	mov	lr, pc
	mov	pc, r3

mnext:
	add	r4, r4, #1		/* move on to next bit */
	cmp	r4, #8			/* done the last bit ? */
	bmi	mloop			/* no - loop back. */
	
	/* Restore the interrupt mask */
	strb    r6, [r7, #IO_ICU1_OCW]
	
mskip:	
	/*
	 * Move to controller #2, and see what bits are set.
	 */
	cmp	r9, #0
	beq	sskip
	
        mov	r4, #0
sloop:
	mov	r3, #1
	mov	r3, r3, lsl r4		/* Build the 1 bit mask */
	tst	r9, r3			/* Is a bit set? */
	beq	snext			/* No, then loop */

	/*
	 * Call the interrupt handler. Be sure to stash the irq number
	 * in the trap frame, and pass the frame pointer as arg0. Note
	 * that the irq numbering needs to be pushed up into 8-15.
	 */
	add	r2, r4, #8
	str     r2, [r10, #(TRAP_STATE_INTNO)]
	ldr	r3, Lbase_irq_handlers
	ldr	r3, [r3, r2, asl #2]	/* r3 = base_irq_handlers[irq] */
	mov	r0, r10
	mov	lr, pc
	mov	pc, r3

snext:
	add	r4, r4, #1		/* move on to next bit */
	cmp	r4, #8			/* done the last bit ? */
	bmi	sloop			/* no - loop back. */
	
	/* Restore the interrupt mask */
	strb    r5, [r7, #IO_ICU2_OCW]
	
sskip:
	/* 
	 * Decrement the hardware interrupt nesting counter
	 */
	ldr	r2, Lbase_irq_nest
	ldr	r3, [r2]
	subs	r3, r3, #0x1
	str	r3, [r2]

	/*
	 * Look for softints, base_irq_nest will be zero if one is pending.
	 */
	bne	nosoft			/* Result of sub above */
	
	/*
	 * XXX There should be a test to see if BASE_IRQ_SKIP_SOFTINT is set,
	 * but since we run multiple handlers at once, kinda of a problem
	 * until I get more ambitious and fix up the code above.
	 */
	
	/*
	 * Clear the pending indicator, disable softints, reenable HW ints
	 * and call the softint handler.
	 */
	mov	r3, #0xC0	/* SOFTINT_DISABLED|SOFTINT_CLEARED */
	str	r3, [r2]
	mrs	r3, cpsr_all
	bic	r3, r3, #0x80
	msr	cpsr_all, r3
	mov	r0, r10
	ldr	r3, Lbase_irq_softint_handler
	ldr	r3, [r3]	/* r3 = *base_irq_softint_handler */
	mov	lr, pc
	mov	pc, r3

	/*
	 * Reenable softints and disable HW interrupts
	 */
	ldr	r2, Lbase_irq_nest
	ldr	r3, [r2]
	bic	r3, r3, #0x40	/* SOFTINT_CLEARED */
	str	r3, [r2]
	mrs	r3, cpsr_all
	orr	r3, r3, #0x80
	msr	cpsr_all, r3
	
nosoft:	
	mov	r0, #0
	ldmfd	sp!, {pc}	
	mov	r0, r0

Lisa_iobus_address:	
	.word	EXT(isa_iobus_address)

Lbase_irq_nest:	
	.word	EXT(base_irq_nest)

Lbase_irq_handlers:
	.word	EXT(base_irq_handlers)

Lbase_irq_softint_handler:
	.word	EXT(base_irq_softint_handler)
