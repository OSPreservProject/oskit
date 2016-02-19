/*
 * Copyright (c) 1994-1995, 1998, 1999 University of Utah and the Flux Group.
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
#include <oskit/x86/eflags.h>

#include "trap_asm.h"

	.text
	.code16

ENTRY(i16_real_int)
	pushfl
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp

	/*
	 * Retrieve the trap_state pointer parameter (ebp).
	 * It is important that this pointer be stored in ebp
	 * so that we can easily access the trap_state through it
	 * even after we trash our DS register.
	 */
ADDR32	movl	6*4(%esp),%ebp

	/*
	 * Modify the int instruction below with the appropriate int number.
	 * It is important that interrupts be disabled
	 * from the modifiction until the actual int instruction,
	 * in order to ensure that this routine is reentrant.
	 * (It still isn't MP-safe this way, but neither is the BIOS,
	 * so that's probably not a problem.)
	 */
ADDR32	movb	TR_TRAPNO(%ebp),%al
	cli
	movb	%al,1f+1

	/* Load the appropriate registers from the trap_state */
ADDR32	movl	TR_EAX(%ebp),%eax
ADDR32	movl	TR_EBX(%ebp),%ebx
ADDR32	movl	TR_ECX(%ebp),%ecx
ADDR32	movl	TR_EDX(%ebp),%edx
ADDR32	movl	TR_ESI(%ebp),%esi
ADDR32	movl	TR_EDI(%ebp),%edi
ADDR32	movw	TR_V86_DS(%ebp),%ds
ADDR32	movw	TR_V86_ES(%ebp),%es
ADDR32	andl	$-1-EFL_IF,TR_EFLAGS(%ebp)	/* mustn't enable interrupts */
ADDR32	pushl	TR_EFLAGS(%ebp)
	popfl
ADDR32	movl	TR_EBP(%ebp),%ebp

1:	int	$0

	/* Save the registers back into the trap_state structure */
	pushl	%ebp
ADDR32	movl	7*4(%esp),%ebp
ADDR32	popl	TR_EBP(%ebp)
ADDR32	movl	%eax,TR_EAX(%ebp)
ADDR32	movl	%ebx,TR_EBX(%ebp)
ADDR32	movl	%ecx,TR_ECX(%ebp)
ADDR32	movl	%edx,TR_EDX(%ebp)
ADDR32	movl	%esi,TR_ESI(%ebp)
ADDR32	movl	%edi,TR_EDI(%ebp)
ADDR32	movw	%ds,TR_V86_DS(%ebp)
ADDR32	movw	%es,TR_V86_ES(%ebp)
	pushfl
ADDR32	popl	TR_EFLAGS(%ebp)

	/* Restore our standard data segment registers */
	movw	%ss,%ax
	movw	%ax,%ds
	movw	%ax,%es

	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	popfl
#ifdef HAVE_CODE16GCC
	retl	/* 32-bit return */
#else
	ret	/* 32-bit return */
#endif

#endif /* HAVE_CODE16 */
