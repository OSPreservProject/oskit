/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
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
 * assembly support for anno_test.c
 */

#include <oskit/x86/asm.h>
#include <oskit/x86/anno.h>

/*
 * Copy words.  The copy operation is broken into two operations so we can
 * annotate each seperately and just so we will have more than one annotation!
 * Someone with a clue about the x86 could combine these in a single
 * repeated copy and use the fault info to determine if the read or the
 * write faulted.
 */
	.globl copy_asm
copy_asm:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	pushl	%ebx
	movl	8(%ebp),%ebx
	movl	12(%ebp),%ecx
	movl	16(%ebp),%edx
	xorl	%esi,%esi

loop:
	movl	%edx,%eax
	decl	%edx
	testl	%eax,%eax
	jle	done
	ANNO_TRAP(copy_recover, err)
	movl	(%ebx),%eax
	addl	$4,%ebx
	ANNO_TRAP(copy_recover, err)
	movl	%eax,(%ecx)
	addl	$4,%ecx
	jmp	loop

done:
	movl	%esi,%eax
	leal	-8(%ebp),%esp
	popl	%ebx
	popl	%esi
	movl	%ebp,%esp
	popl	%ebp
	ret
err:
	movl	$1,%esi
	jmp	done

/*
 * Big loop.  Does nothing useful except to spin for a period of time so that
 * it can be interrupted at various points.
 */
	.data
loop_buf:
	.zero	64
	.text
	.globl	loop_asm,loop_start,loop_end
loop_asm:
	pushl	%ebp
	movl	%esp,%ebp
	movl	$1,%eax
	jmp	loop_end
loop_start:
	ANNO_INTR(loop_recover, loop_end)
	incl	loop_buf
	incl	loop_buf+4
	incl	loop_buf+8
	incl	loop_buf+12
	incl	loop_buf+16
	incl	loop_buf+20
	incl	loop_buf+24
	incl	loop_buf+28
	incl	loop_buf+32
	incl	loop_buf+36
	incl	loop_buf+40
	incl	loop_buf+44
	incl	loop_buf+48
	incl	loop_buf+52
	incl	loop_buf+56
	incl	loop_buf+60
	ANNO_INTR(0, 0)
loop_end:
	orl	%eax,%eax
	jnz	loop_start
	movl	%ebp,%esp
	popl	%ebp
	ret
