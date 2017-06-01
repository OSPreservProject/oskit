/*
 * Copyright (c) 1994-1999 University of Utah and the Flux Group.
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

#include <oskit/config.h>
#ifdef HAVE_CODE16

#include <oskit/x86/asm.h>

	.code16
	.text

ENTRY(_start)
	/*
	 * DOS starts us up with our stack pointer pointing
	 * to the very top of our BSS segment.
	 * ds and es point to the PSP.
	 */

#define DELAY jmp 1f; 1: jmp 1f; 1: jmp 1f; 1:

	/*
	 * Check to make sure we're running on a 386 or higher -
	 * _without_ using any 32-bit instructions of course.
	 * Tricky, since gas doesn't support 16-bit addressing modes. :-)
	 * We can't produce any 16-bit relocations either,
	 * because ELF doesn't support them.
	 * This code is basically straight out of the Pentium manual,
	 * except gassed of coursed.
	 */
	pushfw
	DELAY
	popw	%bx
	movw	$0xfff,%ax
	andw	%bx,%ax
	pushw	%ax
	DELAY
	popfw
	DELAY
	pushfw
	DELAY
	popw	%ax
	and	$0xf000,%ax
	cmpw	$0xf000,%ax

	je	1f
	orw	$0xf000,%bx
	pushw	%bx
	DELAY
	popfw
	DELAY
	pushfw
	DELAY
	popw	%ax
	andw	$0xf000,%ax
	jnz	4f
1:
	/* Gak!  We're running on an icky 8086/8088/80286!  */
	callw	5f
	.ascii	"This program requires a 386 or better.\r\n\0"
5:	popw	%si
	movw	%cs,%ax
	movw	%ax,%ds
	cld
2:	lodsb
	orb	%al,%al
	jz	3f
	movb	$0x02,%ah
	movb	%al,%dl
	int	$0x21
	jmp	2b
3:	movw	$0x4c02,%ax
	int	$0x21
4:
	/* Now we can use 32-bit instructions all we want.  */

	/* Save the PSP segment address (dx).  */
	movw	%ds,%dx

	/* Find our real-mode code segment (ax).  */
	movw	%cs,%ax

#define ENABLE_PAGE_ALIGNED_KERNEL
#ifdef ENABLE_PAGE_ALIGNED_KERNEL
	/*
	 * Move our code and data so that everything is on a page boundary.
	 * Theoretically we _could_ go past the end of available memory,
	 * since we're not checking, but it's enormously unlikely.
	 */
	std
	movw	%ax,%ds
	addw	$0xff,%ax
	andw	$0xff00,%ax
	movw	%ax,%es
	movl	$EXT(edata),%ecx
	subl	$EXT(i16_entry_2),%ecx
	movl	$EXT(edata)-1,%esi
	movw	%si,%di
	rep
	movsb

	/* Start running at the new address.  */
	pushl	$EXT(i16_entry_2)
ADDR32	movw	%ax,2(%esp)
	lretw

ENTRY(i16_entry_2)
	/* We're now page aligned.  */
#endif ENABLE_PAGE_ALIGNED_KERNEL

	/* Load the data segment registers appropriately.  */
	movw	%ax,%es
	movw	%ax,%ss

	/* Start using a real stack.  */
	movl	$EXT(base_stack_end),%esp

	/* Clear our BSS segment.  */
	movl	$EXT(edata),%edi
	movl	$EXT(end),%ecx
	subw	%di,%cx
	xorb	%al,%al
	cld
	rep
	stosb

	/*
	 * Find the size of the environment array (si) 
	 * and the number of environment variables plus one (bx).
	 * The PSP segment is still in dx.
	 */
	movw	%dx,%ds
	movw	0x2c,%ds
	xorw	%si,%si
1:	lodsb
	orb	%al,%al
	jnz	1b
	lodsb
	orb	%al,%al
	jnz	1b

	/*
	 * Allocate space for the environment array on the stack.
	 * Also make sure the top 16 bits of ESP are cleared,
	 * and that the stack pointer is longword aligned.
	 */
	subw	%si,%sp
	andl	$0x0000fffc,%esp

	/*
	 * Copy the environment array to the local stack.
	 * We present it backwards, but big deal - shouldn't matter.
	 */
	xorl	%edi,%edi
	movw	%sp,%di
	xorl	%esi,%esi
	pushl	%esi
	jmp	3f
2:	pushl	%edi
	stosb
1:	lodsb
	stosb
	orb	%al,%al
	jnz	1b
3:	lodsb
	orb	%al,%al
	jnz	2b
	movl	%esp,%cs:EXT(environ)

	/*
	 * Copy the program name to the local stack;
	 * it will be used as argv[0].
	 */
	lodsw
	movw	%si,%bx
1:	pushw	$0
	lodsb
	orb	%al,%al
	jz	2f
	lodsb
	orb	%al,%al
	jnz	1b
2:	movw	%bx,%si
	movw	%sp,%di
3:	lodsb
	stosb
	orb	%al,%al
	jnz	3b
	movl	%esp,%ebp

	/*
	 * Build argv[1..n] from the command tail in the PSP.
	 * Count the arguments in ebx.
	 */
	movw	%dx,%ds
	xorl	%ecx,%ecx
	xorl	%ebx,%ebx
	movb	0x80,%cl	/* get size of command tail */
	incw	%cx		/* plus the return character */
	movw	$0x80,%si
	addw	%cx,%si		/* si = ptr to return character */
	movw 	%sp,%di
	decw	%di
	subw	%cx,%sp		/* allocate space on the stack */
	andw	$0xfffc,%sp
	pushl	%ebx
	std
1:	xorb	%al,%al		/* store a null terminator for this arg */
	stosb
	incl	%ebx
2:	cmpw	$0x80,%si
	je	5f
	lodsb			/* scan backwards for the end of an arg */
	cmpb	$0x20,%al
	jbe	2b
3:	stosb			/* copy the arg */
	cmpw	$0x80,%si
	je	4f
	lodsb
	cmpb	$0x20,%al
	ja	3b
4:	movw	%di,%cx		/* push an arg pointer */
	incw	%cx
	pushl	%ecx
	jmp	1b
5:

	/* Push the argv[0] pointer.  */
	pushl	%ebp

	/* Push the argument and envirnonment parameters on the stack.  */
	movl	%esp,%eax
	pushl	%cs:EXT(environ)
	pushl	%eax
	pushl	%ebx

	/*
	 * Release all conventional memory above the top of our BSS.
	 * The PSP segment is still in dx.
	 */
	movl	$EXT(end)+15,%ebx
	shrw	$4,%bx
	movw	%cs,%ax
	addw	%ax,%bx
	subw	%dx,%bx
	movw	%dx,%es
	movb	$0x4a,%ah
	int	$0x21

	/* Load the normal data segment registers.  */
	movw	%cs,%ax
	movw	%ax,%ds
	movw	%ax,%es

	/* GCC wants the direction flag cleared at all times.  */
	cld

	/* Make backtraces terminate */
	xorl	%ebp,%ebp

	/* Initialize the bss and run the program.  */
	call	EXT(i16_dos_main)

	.globl	EXT(environ)
	.comm	EXT(environ),4


	.data

#ifdef __ELF__
	.section .anno,"aw",@progbits
	P2ALIGN(4)
	.globl	__ANNO_START__
__ANNO_START__:
#endif

#endif /* HAVE_CODE16 */
