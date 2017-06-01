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
 * Copyright (c) 1994-1997 Mark Brinicombe.
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 */

#include <oskit/arm32/asm.h>
#include <oskit/arm32/trap.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/proc_reg.h>
#include <oskit/page.h>

#define SHARP #

/*
 * All traps enter through this stub. We save away 5 registers on the
 * current mode stack so that we can load one with a trap number. The
 * other 4 are used in the common routine below.
 */
#define	TRAP(n, name)						\
	.align	0						;\
GLEXT(name)							;\
	stmdb	sp, {r0-r4}					;\
	mov	r4, SHARP( n )					;\
	b	alltraps					;\
	mov	r0,r0
	
	.text
	
/*
 * Here are the stubs.
 */
TRAP(T_UNDEF, t_undef)
TRAP(T_SWI, t_swi)
TRAP(T_PREFETCH_ABORT, t_prefetch_abort)
TRAP(T_DATA_ABORT, t_data_abort)
TRAP(T_ADDREXC, t_addrexc)
TRAP(T_IRQ, t_irq)
TRAP(T_FIQ, t_fiq)
	
/*
 * This is a table of entrypoints for the above stubs. This table gets
 * plugged into the trap vector address table in page zero.
 */
	.data
	P2ALIGN(4)
GLEXT(base_trap_vectors)
	.word	0		/* Reset is special! See t_reset below */
	.word	t_undef
	.word	t_swi
	.word	t_prefetch_abort
	.word	t_data_abort
	.word	t_addrexc
	.word	t_irq
	.word	t_fiq

	.text

/*
 * This is the common routine for all traps.
 *
 * NOTE: The lr register is *NOT* adjusted, since the amount of adjustment
 * depends on the type of trap. 
 */
	.align	0
alltraps:
	mov	r0, lr			/* Save xxx32 r14 */
	mov	r1, sp			/* Save xxx32 sp */
	mrs	r3, spsr_all		/* Save xxx32 spsr */
	mrs     r2, cpsr_all 		/* Get the CPSR */
	bic     r2, r2, #(PSR_MODE)	/* Fix for SVC mode */
	orr     r2, r2, #(PSR_SVC32_MODE)
	msr     cpsr_all, r2		/* Punch into SVC mode */
	/*
	 * Poor Mans Stack Overflow Detection. Compare the PC of the
	 * offending instruction against the PC of Lbadpc. On a stack
	 * overflow, the first push instruction (2 instructions before
	 * Lbadpc) will fault, and we can catch that in the second trap.
	 * If that happens, change the trap number to T_STACK_OVERFLOW,
	 * and change the stack to the overflow stack. The trap handler
	 * will have to be aware that the stack is special though, and
	 * and not do anything that would cause a thread block/switch.
	 */
	ldr     r2, Lbadpc
	cmp     r0, r2
	bne	Lokay
	mov	r4, #T_STACK_OVERFLOW
	mov	r2, sp			/* Save	actual SVC sp */
	ldr	sp, Loverflowstack
	b	Loflo
Lokay:	
	mov	r2, sp			/* Save	SVC sp */
Loflo:
	str	r0, [sp, #-4]!		/* Push return address */
	str	lr, [sp, #-4]!		/* Push SVC lr */
badpc:	
	str	r2, [sp, #-4]!		/* Push SVC sp */
	str	r4, [sp, #-4]!		/* Push interrupt request num slot */
	str	r4, [sp, #-4]!		/* Push trap number */
	msr     spsr_all, r3		/* Restore correct spsr */
	ldmdb	r1, {r0-r4}		/* Restore 5 regs from xxx mode */
	sub	sp, sp, #(4*15)		/* Adjust the stack pointer */
	stmia	sp, {r0-r12}		/* Push the user mode registers */
	add	r0, sp, #(4*13)		/* Adjust the stack pointer */
	stmia	r0, {r13-r14}^		/* Push the user mode registers */
        mov     r0, r0                  /* NOP for previous instruction */
	mrs	r0, spsr_all		/* Put the SPSR on the stack */
	str	r0, [sp, #-4]!

	/*
	 * Index into the trap table to get the proper handler.
	 */
	ldr	r3, Lhandlers
	ldr	r4, [sp, #(TRAP_STATE_TRAPNO)]	/* reload the trapno */
	ldr	r3, [r3, r4, asl #2]

	/*
	 * Call the trap handler, passing the stack pointer as arg0.
	 */
	mov	r0, sp
	mov	lr, pc
	mov	pc, r3

	/*
	 * Back from trap handler. If the handler function returned zero 
	 * (success), then resume execution as if the trap never happened.
	 * Otherwise, just panic.
	 */
	cmp	r0, #0
	beq	justreturn
	mov	r0, sp
	bl	EXT(trap_dump_panic)

	/*
	 * Pull all the goo from the stack and return.
	 */
justreturn:
        ldr     r0, [sp], #0x0004	/* Get the SPSR from stack */
        msr     spsr_all, r0		/* restore SPSR */
        ldmia   sp, {r0-r14}^		/* Restore registers (usr mode) */
        mov     r0, r0	  		/* NOP for previous instruction */
	add	sp, sp, #(4*17)		/* Adjust the stack pointer (+trapno)*/
	ldmia	sp, {sp, lr, pc}^	/* Restore lr and exit */
	mov	r0,r0

Lhandlers:
	.word	EXT(base_trap_handlers)

Loverflowstack:
	.word	EXT(overflow_stack)

Lbadpc:
	.word	badpc

/*
 * t_reset: Reset is special because it does not come in as special processor
 * mode, and because the MMU is disabled. When setting the trap vector in
 * page 0, be sure to use the physical address.
 *
 * XXX:	Not used on the SHARK 'cause it does not work as advertised.
 */
ENTRY(t_reset)
	/*
	 * Turn the MMU back on! We lose register r1, but such is life.
	 */
	mrc	15, 0, r1, c1, c0, 0
	orr	r1, r1, #(CPU_CTRL_M|CPU_CTRL_C|CPU_CTRL_W)
	mcr	15, 0, r1, c1, c0, 0
	nop
	nop
	nop
	nop

	/*
	 * Push the frame.
	 */
	str	lr, [sp, #-4]!		/* Push the return address */
	sub	sp, sp, #(4*19)		/* Adjust the stack pointer */
	stmia	sp, {r0-r12};		/* Push the user mode registers */
	add	r0, sp, #(4*13)		/* Adjust the stack pointer */
	stmia	r0!,{r13-r14}^		/* Push the user mode registers */
        mov     r0, r0                  /* NOP for previous instruction */
	mov	r1, #0
	str	r1, [r0, #-4]!		/* Push trapno (reset is 0) */
	str	r1, [r0, #-4]!		/* No interrupt */
	str	r1, [r0, #-4]!		/* No svc_sp */
	str	r1, [r0, #-4]!		/* No svc_lr */
	mrs	r0, spsr_all		/* Put the SPSR on the stack */
	str	r0, [sp, #-4]!

	/*
	 * Call the C trap handler, passing the stack pointer as arg0.
	 */
	mov	r0, sp
	b	EXT(trap_dump_panic)

	/*
	 * Never returns.
	 */
	
/*
 * Each mode the processor enters (data abort, irq, svc, etc.) has its own
 * stack pointer. That stack is used to aid in switching to SVC mode.
 *
 * To set the stack pointer for a particular mode we must switch
 * to that mode update the banked r13 and then switch back.
 * This routine provides an easy way of doing this for any mode
 *
 * r0 = CPU mode
 * r1 = stackptr
 */
ENTRY(set_stackptr)
        mrs	r3, cpsr_all		/* Switch to the appropriate mode */
	bic	r2, r3, #(PSR_MODE)
	orr	r2, r2, r0
        msr	cpsr_all, r2

	mov	sp, r1			/* Set the stack pointer */

        msr	cpsr_all, r3		/* Restore the old mode */

	mov	pc, lr			/* Exit */

/*
 * Allocate stacks for each of the modes we are interested in. Since these
 * stacks do little more than provide a place to stash some stuff when
 * switching to SVC mode, they can be very small. 
 */
	.bss
	P2ALIGN(PAGE_SHIFT)

		.space 128
	GLEXT(abort_stack)
		.space 128
	GLEXT(irq_stack)
		.space 128
	GLEXT(fiq_stack)
		.space 128
	GLEXT(undef_stack)

/*
 * The overflow stack is a software defined stack, which is used when the
 * trap routine above detects stack overflow.
 */
		.space 4096
	GLEXT(overflow_stack)
		.space 4

