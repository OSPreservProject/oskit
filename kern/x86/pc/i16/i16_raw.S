/*
 * Copyright (c) 1994-1998, 2000 University of Utah and the Flux Group.
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
	This file rovides a default implementation
	of real/pmode switching code.
	Assumes that, as far as it's concerned,
	low linear address always map to physical addresses.
	(The low linear mappings can be changed,
	but must be changed back before switching back to real mode.)

	Provides:
		i16_raw_switch_to_pmode()
		i16_raw_switch_to_real_mode()

		i16_raw_start()
			Called in real mode.
			Initializes the pmode switching system,
			switches to pmode for the first time,
			and calls the 32-bit function raw_start().

	Depends on:

		paging.h:
			raw_paging_enable()
			raw_paging_disable()
			raw_paging_init()

		a20.h:
			i16_enable_a20()
			i16_disable_a20()

		real.h:
			real_cs
*/
#include <oskit/config.h>
#ifdef HAVE_CODE16
			
#include <oskit/x86/asm.h>

#include "pc_asm.h"
#include "x86_asm.h"
	

#ifdef HAVE_CODE16GCC
#define GASCODE16 .code16gcc
#else
#define GASCODE16 .code16
#endif			
	
.text
.code32

#ifdef KNIT
/*
 * This 32-bit function is the last thing called when the system
 * shuts down.  It switches back to 16-bit mode and calls i16_exit().
 */
/* void exit32(int rc) */
ENTRY(exit32)
#else
/*
 * This 32-bit atexit handler is registered on startup;
 * it switches back to 16-bit mode and calls i16_exit().
 * Unfortunately we can't pass the true return code
 * because C's standard atexit mechanism doesn't provide it.
 */
/* static void raw_atexit(void) */
raw_atexit:
#endif	
	pushl %ebp
	movl %esp, %ebp
	
	ljmp	$KERNEL_16_CS, $1f
GASCODE16
	1:
	
	call i16_raw_switch_to_real_mode

#ifdef KNIT
	/* Leave the stack as we found it; just pass on our argument. */
	leave
#else	
	pushl $0
#endif	
	call EXT(i16_exit)

	

.text
GASCODE16

/* void i16_raw_start(void (*start32)(void) OSKIT_NORETURN) */
	
ENTRY(i16_raw_start)	
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx

	movl 8(%ebp), %ebx

	/* Make sure we're not already in protected mode.  */	
	smsw %ax
	testb $CR0_PE, %al

	je real_mode

	/* PE bit set, time to panic. */
	pushl $panicstr
	call EXT(i16_abort)

real_mode:
	/* Minimally initialize the GDT.  */	
	call EXT(i16_base_gdt_init)

	/* Switch to protected mode for the first time.  
	 * This won't load all the processor tables and everything
	 * yet, since *they're not fully initialized.  
	 */
	call i16_raw_switch_to_pmode

	ljmp	$KERNEL_CS, $1f
		.code32
	1:
	
#ifndef KNIT
	/* Make sure we exit properly when we do */
	pushl $raw_atexit
	call EXT(atexit)
	addl $4, %esp
#endif /* !KNIT */	

	/* Initialize the oskit's base processor tables.  */
	call EXT(base_cpu_init)

	movb $1, inited

	ljmp $KERNEL_16_CS, $1f
		GASCODE16
	1:

	/* Switch to real mode and back again once more,
	 * to make sure everything's loaded properly.  
	 */
	call i16_raw_switch_to_real_mode
	call i16_raw_switch_to_pmode

	ljmp	$KERNEL_CS, $1f
		.code32
	1:
	
	call *%ebx


GASCODE16		

/* void i16_raw_switch_to_pmode(void) */

ENTRY(i16_raw_switch_to_pmode)
	pushl %ebp
	movl %esp, %ebp

	/* Save some space for a pesudo_descriptor. */	
	subl $8, %esp

	/* No interrupts from now on please.  */	
	cli

	/* Save the eflags register for switching back later.  */	
	pushfl
	popl %ecx
	movl %ecx, EXT(real_eflags)

	/* Enable the A20 address line.  */	
	movl EXT(base_i16_enable_a20), %eax
	call *%eax

	/* Load our protected-mode GDT.  Note that we have to do this
	 * each time we enter protected mode, not just the first, because
	 * other real-mode programs may have switched to pmode and back
	 * again in the meantime, trashing the GDT pointer.  
	 */

	/* Set limit: base_gdt has GDTSZ descriptors, of 8 bytes each. */

	movw	$(BASE_GDT_SIZE - 1), %ax
	movw    %ax, -6(%ebp)

	/* Set base; remember to kvtolin. */
	movl $EXT(base_gdt), %edx
	leal -8(%ebp), %eax
ADDR32	subl EXT(linear_base_va), %edx
	movl %edx, 4(%eax)

	lgdt 2(%eax)

	/* Switch into protected mode.  */
	mov	%cr0, %eax
	orb	$CR0_PE, %al
	movl	%eax, %cr0

	prot_jmp:
		ljmp $KERNEL_16_CS, $1f
	1:

	/* Load all of the segment registers. */	
	movw $KERNEL_DS, %cx
	movw %cx, %ds
	movw %cx, %es
	movw %cx, %ss

	xorl %ecx, %ecx
	movw %cx, %fs
	movw %cx, %gs

	ljmp	$KERNEL_CS, $1f
		.code32
	1:
	
	cmpb $0, inited
	je already_inited

	call EXT(base_cpu_load)

	pushl $BASE_IRQ_SLAVE_BASE
	pushl $BASE_IRQ_MASTER_BASE
	call EXT(pic_init)

already_inited:	

	/* Make sure our flags register is appropriate.  */	
	pushfl
	popl %eax

	andb $0xb9, %ah		/* ~(EFL_IF | EFL_DF | EFL_NT) */
	orb $0x30, %ah		/* EFL_IOPL_USER */
	
	pushl %eax
	popfl

	ljmp	$KERNEL_16_CS,$1f
		GASCODE16
	1:

	leave
	ret

	

GASCODE16

/* void i16_raw_switch_to_real_mode(void) */

ENTRY(i16_raw_switch_to_real_mode)
	pushl %ebp
	movl %esp, %ebp

	/* Reserve some space for a pseudo_descriptor. */
	subl $8, %esp

	/* Make sure interrupts are disabled.  */	
	cli

	/* Avoid sending DOS bogus coprocessor exceptions.
	 * XXX should we save/restore all of CR0?  
	 */	
	clts
	
	ljmp $KERNEL_CS, $1f
		.code32
	1:

	/* Reprogram the PIC back to the settings DOS expects.  */	
	pushl $0x70
	pushl $8
	call EXT(pic_init)
	addl $8, %esp
	
	ljmp $KERNEL_16_CS, $1f
		GASCODE16
	1:
	
	/* Make sure all the segment registers are 16-bit.
	 * The code segment definitely is already,
	 * because we're running 16-bit code.  
	 */		
	movw $KERNEL_16_DS, %dx
	movw %dx, %ds
	movw %dx, %es
	movw %dx, %fs
	movw %dx, %gs
	movw %dx, %ss

	/* Switch back to real mode.  */	

	/* Turn off PE flag. */
	movl %cr0, %eax
	andb $~(CR0_PE), %al
	movl %eax, %cr0

	jmp	1f
1:

ADDR32	pushw EXT(real_cs)
	pushw $1f

foo:		
#ifdef HAVE_CODE16GCC
	ljmp	(%esp)
#else
	.byte	0x67, 0xff, 0x2c, 0x24		/* 16 bit ljmp, not 32! */
#endif		
1:

	/* Load the real-mode segment registers.  */		
ADDR32	movw EXT(real_cs), %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss

	/* Initialize the real-mode IDT pseudo descriptor. */
	leal -8(%ebp), %eax
	orl $0xffff0000, (%eax)		/* limit */
	movl $0, 4(%eax)		/* linear_base */

	/* Load the real-mode IDT.  */	
	lidt 2(%eax)

	/* Disable the A20 address line.  */	
	movl EXT(base_i16_disable_a20), %eax
	call *%eax

	/* Restore the eflags register to its original real-mode state.
	 * Note that this will leave interrupts disabled
	 * since it was saved after the cli() above.  
	 */	
	movl EXT(real_eflags), %edx
	pushl %edx
	popfl

	leave
	ret


.data
		
GLEXT(base_i16_switch_to_real_mode)
	.long i16_raw_switch_to_real_mode

GLEXT(base_i16_switch_to_pmode)
	.long i16_raw_switch_to_pmode

inited:	
	.byte 0
real_eflags:	
	.long 0

P2ALIGN(5)	
panicstr:
	.string	"The processor is in an unknown protected mode environment."


#endif /* HAVE_CODE16 */
