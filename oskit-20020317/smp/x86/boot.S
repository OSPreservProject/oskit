/*
 * Copyright (c) 1996-1999 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit SMP Support Library, which is free software,
 * also known as "open source;" you can redistribute it and/or modify it under
 * the terms of the GNU General Public License (GPL), version 2, as published
 * by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

#include <oskit/config.h>
#include <oskit/x86/asm.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/base_gdt.h>

#ifdef HAVE_CODE16 /* SMP startup requires 16-bit assembly code support */

#undef CONSOLE_DEBUG
#undef SMP_DEBUG
#define LOOP_COUNT 50


/*
 * This is the code that is executed as soon as the Application
 * Processors are started in Real mode.  It does enough setup
 * to call C-code in protected mode.
 */

	.text
	.code16

ENTRY(_SMP_TRAMP_START_)

/* This is the first instruction executed after it is started up! */
	cli
	cld
	xorl	%eax, %eax
	movl	%eax, %ebp
	movl	%eax, %edx

/* load stack pointer (only good while in 16-bit, so why bother?) */
	movw	%cs, %ax
	movw	%ax, %ds	/* set up ds for convenience */
	movw	%ax, %dx	/* for protected mode stack... */

	movw	%ax, %ss
	/* point to end of page */
	movw	$4096, %ax
	movl	%eax, %esp



#ifdef CONSOLE_DEBUG

	movw $0xb800, %ax
	movw %ax, %es
	movl $79*2, %edi
	xorw %eax, %eax

L3:
        movb %al, %bl
        and $0xf, %bl
        movb %bl, %es:1(%edi)
        movw $5000, %cx
L5:
        mov %al, %es:(%edi)
        loop L5

        addw $160, %di
        cmpw $80*25*2 , %di

        jle L4
        movw $79*2, %di
L4:
        incw %ax

        cmpw $LOOP_COUNT, %ax
        jne L3

#endif

	/* okay, now load the gdt/idt/tss/etc. */

	/* Load the GDT. */
	lgdt	EXT(smp_boot_gdt_limit) - EXT(_SMP_TRAMP_START_)

	/* go to protected mode */
	movl	%cr0, %eax
	orl	$CR0_PE,%eax	/* DUH! */
	movl	%eax, %cr0
	/* in protected mode!  Now go to 32-bit segment
	 * Note that it is essential that we use `ljmpl'
	 * here instead of `ljmp', so that the target
	 * address gets a proper 32-bit relocation entry.
	 */
	ljmpl	$KERNEL_CS, $EXT(_SMP_TRAMP_32_ENTRY_)

	.code32

ENTRY(_SMP_TRAMP_32_ENTRY_)

/*
 * [_SMP_TRAMP_32_ENTRY_ - 6] is the 32 bit address of the ljmp destination.
 * It can be patched in C code before the page is copied!
 * If it isn't, then I jump to the ORIGINAL code!
 */

	/* Reload all the segment registers from the new GDT.  */
	movw	$KERNEL_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	/* XXX  %esp is off... */

	xorl	%eax, %eax	/* no prefix with eax */
	movw	%ax, %fs	/* init both to 0 */
	movw	%ax, %gs

	sall	$4, %edx	/* multiply by 16 */
	movl	%edx, %ebx	/* ebx is start of trampoline page */
	addl	$4096, %edx

	movl	%edx, %esp	/* stack is at end of page... */

	/* set up idt */
	lidt	EXT(smp_boot_idt_limit)-EXT(_SMP_TRAMP_START_)(%ebx)

	/* set up tss */
	/*	ltr	BASE_TSS	call C code later... */


	/* Make sure our flags register is appropriate.  */
	pushl	$0	/* ??? */
	popf

	/* Now I can call the C routine to set up a real stack and stuff */

	/* shoot, have C set up the TSS */
	call	EXT(base_tss_load)

	movl	%ebx, %esi	/* ebx, edi, esi saved by gcc */
#ifdef SMP_DEBUG
	addl	$smp_boot_message-EXT(_SMP_TRAMP_START_), %ebx

	/* printf MUST do some mutual-exclusion at some level. (putc) */
	pushl	%ebx
	call	EXT(printf)
	addl	$4, %esp
#endif /* SMP_DEBUG */

	/* let BSP know I've booted! */
	call	EXT(smp_callin)

#ifdef SMP_DEBUG
	movl	%esi, %ebx
	addl	$smp_boot_message2-EXT(_SMP_TRAMP_START_), %ebx

	pushl	%ebx
	call	EXT(printf)
	addl	$4, %esp
#endif /* SMP_DEBUG */

	movl	%esi, %ebx
	addl	$EXT(smp_boot_spin)-EXT(_SMP_TRAMP_START_), %ebx

	xorl	%ecx, %ecx	/* spin while zero */
spin_loop:
	movl	(%ebx), %eax
	cmpl	%ecx, %eax
	je	spin_loop

#ifdef SMP_DEBUG
	movl	%esi, %eax
	addl	$smp_boot_message3-EXT(_SMP_TRAMP_START_), %eax

	pushl	%eax
	call	EXT(printf)
	addl	$4, %esp
#endif /* SMP_DEBUG */

	/* set up the stack pointer */
	movl	%esi, %eax
	addl	$EXT(smp_boot_stack)-EXT(_SMP_TRAMP_START_), %eax
	movl	(%eax), %esp

	/* push the data (pointer) onto the new stack */
	movl	%esi, %eax
	addl	$EXT(smp_boot_data)-EXT(_SMP_TRAMP_START_), %eax
	pushl	(%eax)

	/* extract the function pointer to call */
	movl	%esi, %eax
	addl	$EXT(smp_boot_func)-EXT(_SMP_TRAMP_START_), %eax

	/*
	 * Now we are through with the trampoline page.
	 * Tell the BSP we don't need it any more.
	 */
	addl	$EXT(smp_boot_ack)-EXT(_SMP_TRAMP_START_), %esi
	movl	%eax, (%esi)	/* %eax happens to be nonzero, good enough */

	/*
	 * Call the function, now running free and clear on the given stack.
	 */
	call	*(%eax)
	addl	$4, %esp	/* pop function arg */

	/* XXX should give a warning that the routine returned! */

	hlt

/*
 * Variables
 */

	P2ALIGN(2)

				.word	0
GLEXT(smp_boot_gdt_limit)	.word	0
GLEXT(smp_boot_gdt_linear_base)	.long	0

				.word	0
GLEXT(smp_boot_idt_limit)	.word	0
GLEXT(smp_boot_idt_linear_base)	.long	0

GLEXT(smp_boot_func)		.long 0 /* function pointer */
GLEXT(smp_boot_data)		.long 0	/* single param (addr) for func */
GLEXT(smp_boot_stack)		.long 0	/* stack pointer for function */

GLEXT(smp_boot_spin)		.long 0	/* spin until not zero, then load ^ */
GLEXT(smp_boot_ack)		.long 0	/* zero this when off initial stack */

#ifdef SMP_DEBUG
/* This can be sprintf'ed to in C before the page is copied -- or even after */
smp_boot_message:	.ascii "This is the second processor!\12\12\0"
smp_boot_message2:	.ascii "... back to assembly\12\0"
smp_boot_message3:	.ascii "... loading stack pointer\12\0"
#endif /* SMP_DEBUG */

	P2ALIGN(2)

GLEXT(_SMP_TRAMP_END_)
/* This is the end label, so I know how much to copy down to low memory. */


#endif /* HAVE_CODE16 */
