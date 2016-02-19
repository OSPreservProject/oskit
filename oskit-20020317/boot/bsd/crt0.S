/*
 * Copyright (c) 1994-1996, 1998 University of Utah and the Flux Group.
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

#include <oskit/machine/asm.h>

#include "boottype.h"

#define STACK_SIZE 4096

	.globl	_start, EXT(boottype), EXT(boothowto), EXT(bootdev)

	.data

/* Stick these in the data section, because if they're in the BSS
 * they're going to be clobbered.
 */
LEXT(boottype)	.long	0
LEXT(boothowto)	.long	0
LEXT(bootdev)	.long	0

/*
 * On entry, assumes that CS is a 32-bit flat code segment,
 * and DS and ES are a 32-bit flat data segment,
 * all with offset=0 and limit=4GB.
 */
	.text
_start:

	/* In the original Mach bootblocks, this was the howto field,
	 * which should never be zero, since KERNEL_BOOT_ADDR was part 
	 * of howto.  Both the NetBSD and FreeBSD bootblocks pass a 0 
	 * in this field.
	 */
	cmpl    $0,12(%esp)
        je      bsdboot

machboot:
	movl	$BOOTTYPE_MACH,EXT(boottype)
	movl 	12(%esp),%eax		
	movl 	%eax,EXT(boothowto)
	movl	(%esp),%eax
	movl	%eax,EXT(bootdev)
	jmp 	genboot

bsdboot:
	movl	$BOOTTYPE_OLDBSD,EXT(boottype)

	/* Check to see if we've got a return address.  The old BSD 
	 * bootblocks passed a 0 for the return address, but the new-
	 * style FreeBSD bootblocks pass one that's valid.
	 * XXX devs
	 */
	cmpl    $0,(%esp)
	je      oldbsdboot

newbsdboot:
	/* From FreeBSD.  Someday maybe we'll deal with the
	 * bootinfo structure that we've got access to here.  For now
	 * we just fall through, since the fields that we care about 
	 * are in the same spot for both.
	 */

	movl	$BOOTTYPE_NEWBSD,EXT(boottype)

oldbsdboot:
	movl 	4(%esp),%eax
	movl	%eax,EXT(boothowto)
	movl 	8(%esp),%eax
	movl	%eax,EXT(bootdev)

genboot:
	/* Get ourselves a small initial stack.  */
	mov	%ds,%ax
	mov	%ax,%ss
	movl	$stack+STACK_SIZE,%esp
	.comm	stack,STACK_SIZE

	/* Clear the eflags register to a known state.  */
	pushl	$0
	popf

#if 0 /* XXX needed anymore? */
	/* Move the image into its correct position if it isn't in it already.
	   Assumes the new and old positions don't overlap.  */
	call	1f
1:
	popl	%esi
	lea	1b,%edi
	cmpl	%esi,%edi
	jz	2f
	lea	_edata,%ecx
	subl	%edi,%ecx
	repl
	movsb
	lea	2f,%eax
	jmp	*%eax
2:
#endif

	/* Clear uninitialized data.  */
	lea	_edata,%edi
	lea	_end,%ecx
	subl	%edi,%ecx
	xorl	%eax,%eax
	cld
	rep
	stosb

	/* Start the boot program.  */
	jmp	EXT(raw_start)


	/* Certain boot loaders (ahem!)
	   refuse to load bytes 0x400-0x500 of the kernel image,
	   even if the kernel image is linked for 0x100000 instead of 0!
	   Grrrrrrrrrrrrr....  */
	.org	0x500

