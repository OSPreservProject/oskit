/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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
 * Interrupt handler initialization table and stubs.
 */

#include <oskit/x86/asm.h>
#include <oskit/x86/gate_init.h>
#include <oskit/x86/seg.h>
#include <oskit/x86/base_trap.h>
#ifdef GPROF
#include <oskit/x86/pc/irq_list.h>
#endif

#include "pc_asm.h"

/*
 * XXX these should either be generated (ala genassym) or defined in
 * base_trap.h.
 */
#define TS_ERR	52
#define TS_EIP	56
#define TS_CS	60

	.text

/*
 * Gives gprof a symbol to assign time to for functions registered
 * as interrupt handlers
 */
NON_GPROF_ENTRY(oskit_base_irq_handler);


#define MASTER(irq, num)						\
	GATE_ENTRY(BASE_IRQ_MASTER_BASE + (num), 0f, ACC_PL_K|ACC_INTR_GATE) ;\
	P2ALIGN(TEXT_ALIGN)						;\
0:									;\
	pushl	$(irq)			/* error code = irq vector */	;\
	pushl	$BASE_IRQ_MASTER_BASE + (num)	/* trap number */	;\
	pusha				/* save general registers */	;\
	movl	$(irq),%ecx		/* irq vector number */		;\
	movb	$1 << num,%dl		/* pic mask for this irq */	;\
	jmp	master_ints

#define SLAVE(irq, num)						\
	GATE_ENTRY(BASE_IRQ_SLAVE_BASE + (num), 0f, ACC_PL_K|ACC_INTR_GATE) ;\
	P2ALIGN(TEXT_ALIGN)						;\
0:									;\
	pushl	$(irq)			/* error code = irq vector */	;\
	pushl	$BASE_IRQ_SLAVE_BASE + (num)	/* trap number */	;\
	pusha				/* save general registers */	;\
	movl	$(irq),%ecx		/* irq vector number */		;\
	movb	$1 << num,%dl		/* pic mask for this irq */	;\
	jmp	slave_ints


GATE_INITTAB_BEGIN(base_irq_inittab)

MASTER(0, 0)
MASTER(1, 1)
MASTER(2, 2)
MASTER(3, 3)
MASTER(4, 4)
MASTER(5, 5)
MASTER(6, 6)
MASTER(7, 7)
SLAVE( 8, 0)
SLAVE( 9, 1)
SLAVE(10, 2)
SLAVE(11, 3)
SLAVE(12, 4)
SLAVE(13, 5)
SLAVE(14, 6)
SLAVE(15, 7)

GATE_INITTAB_END

	P2ALIGN(TEXT_ALIGN)
master_ints:

	/* Save the current master PIC mask (bl) */
	inb	$0x21,%al
	movb	%al,%bl

	/* Mask the interrupting IRQ */
	orb	%dl,%al
	outb	%al,$0x21

	/* Acknowledge the interrupt */
	movb	$0x20,%al
	outb	%al,$0x20

	/* Save the rest of the standard trap frame (oskit/x86/base_trap.h). */
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	/* Load the kernel's segment registers.  */
	movw	%ss,%dx
	movw	%dx,%ds
	movw	%dx,%es

	/* Increment the hardware interrupt nesting counter */
	incb	EXT(base_irq_nest)

	/* Load the handler vector */
	movl	EXT(base_irq_handlers)(,%ecx,4),%esi
	
	/* XXX re-enable the processor's I flag? */

	/* GCC likes the direction flag cleared.  */
	cld

#ifdef __ELF__	/* XXX only work for ELF right now */
	/*
	 * If supervisor mode, check for interrupt annotations.
	 * If the table is not empty, we find the entry covering the EIP
	 * and call the annotation handler before calling the interrupt
	 * handler.
	 *
	 * XXX only at nest level 0?
	 */
	testb	$3,TS_CS(%esp)
	jnz	1f
	cmpl	$0,EXT(anno_intr)
	jz	1f
	pushl	TS_EIP(%esp)
	pushl	$EXT(anno_intr)
	call	EXT(anno_find_lower)
	addl	$8,%esp
	orl	%eax,%eax
	jz	1f
	movl	4(%eax),%edx
	orl	%edx,%edx
	jz	1f
	pushl	%esp
	pushl	%eax
	call	*%edx
	addl	$8,%esp
#endif /* __ELF__ */

1:
#ifdef FULL_STACK_TRACE
	/*
	 * Fake a stack frame for back traces
	 */
	movl	%esp,%edx
	pushl	TS_EIP(%esp)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edx
	call	*%esi
	movl	%ebp,%esp
	popl	%ebp
	popl	%edx
#else
	/* Call the interrupt handler with the trap frame as a parameter */
	pushl	%esp
	call	*%esi
	popl	%edx
#endif
	movl	%eax,%edx

	/* Unmask the IRQ if the return value from the handler allows it */
	testl	$BASE_IRQ_LEAVE_MASKED,%edx
	jne	1f
	movb	%bl,%al
	outb	%al,$0x21
1:
	/* Decrement the nesting counter and check for software interrupts */
	decb	EXT(base_irq_nest)
	jnz	1f
	
	/* But only if allowed to do so */
	testl	$BASE_IRQ_SKIP_SOFTINT,%edx
	jne	1f
	movb	$BASE_IRQ_SOFTINT_CLEARED|BASE_IRQ_SOFTINT_DISABLED,EXT(base_irq_nest)
	sti
	pushl	%esp
	movl	EXT(base_irq_softint_handler),%ebx
	call	*%ebx
	popl	%eax
	cli
	andb	$BASE_IRQ_SOFTINT_CLEARED,EXT(base_irq_nest)
1:

	/* Return from the interrupt */
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	addl	$4*2,%esp	/* Pop trap number and error code */
	iret


	P2ALIGN(TEXT_ALIGN)
slave_ints:

	/* Save the current slave PIC mask (bl) */
	inb	$0xa1,%al
	movb	%al,%bl

	/* Mask the interrupting IRQ */
	orb	%dl,%al
	outb	%al,$0xa1

	/* Acknowledge the interrupt at both the master and the slave */
	movb	$0x20,%al
	outb	%al,$0x20
	outb	%al,$0xa0

	/* Save the rest of the standard trap frame (oskit/x86/base_trap.h). */
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	/* Load the kernel's segment registers.  */
	movw	%ss,%dx
	movw	%dx,%ds
	movw	%dx,%es

	/* Increment the hardware interrupt nesting counter */
	incb	EXT(base_irq_nest)

	/* Load the handler vector */
	movl	EXT(base_irq_handlers)(,%ecx,4),%esi
	
	/* XXX re-enable the processor's I flag? */

	/* GCC likes the direction flag cleared.  */
	cld

#ifdef __ELF__	/* XXX only work for ELF right now */
	/*
	 * If supervisor mode, check for interrupt annotations.
	 * If the table is not empty, we find the entry covering the EIP
	 * and call the annotation handler before calling the interrupt
	 * handler.
	 *
	 * XXX only at nest level 0?
	 */
	testb	$3,TS_CS(%esp)
	jnz	1f
	cmpl	$0,EXT(anno_intr)
	jz	1f
	pushl	TS_EIP(%esp)
	pushl	$EXT(anno_intr)
	call	EXT(anno_find_lower)
	addl	$8,%esp
	orl	%eax,%eax
	jz	1f
	movl	4(%eax),%edx
	orl	%edx,%edx
	jz	1f
	pushl	%esp
	pushl	%eax
	call	*%edx
	addl	$8,%esp
#endif /* __ELF__ */

1:
#ifdef FULL_STACK_TRACE
	/*
	 * Fake a stack frame for back traces
	 */
	movl	%esp,%edx
	pushl	TS_EIP(%esp)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edx
	call	*%esi
	movl	%ebp,%esp
	popl	%ebp
	popl	%edx
#else
	/* Call the interrupt handler with the trap frame as a parameter */
	pushl	%esp
	call	*%esi
	popl	%edx
#endif
	movl	%eax,%edx

	/* Unmask the IRQ if the return value from the handler allows it */
	testl	$BASE_IRQ_LEAVE_MASKED,%edx
	jne	1f
	movb	%bl,%al
	outb	%al,$0xa1
1:
	/* Decrement the nesting counter and check for software interrupts */
	decb	EXT(base_irq_nest)
	jnz	1f
	/* But only if allowed to do so */
	testl	$BASE_IRQ_SKIP_SOFTINT,%edx
	jne	1f
	movb	$BASE_IRQ_SOFTINT_CLEARED|BASE_IRQ_SOFTINT_DISABLED,EXT(base_irq_nest)
	sti
	pushl	%esp
	movl	EXT(base_irq_softint_handler),%ebx
	call	*%ebx
	popl	%eax
	cli
	andb	$BASE_IRQ_SOFTINT_CLEARED,EXT(base_irq_nest)
1:

	/* Return from the interrupt */
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	addl	$4*2,%esp	/* Pop trap number and error code */
	iret
