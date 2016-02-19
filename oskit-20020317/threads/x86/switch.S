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
 * Copyright (c) UNIX System Laboratories, Inc.  All or some portions
 * of this file are derived from material licensed to the
 * University of California by American Telephone and Telegraph Co.
 * or UNIX System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Task switch code.
 */
#include <oskit/config.h>
#include <oskit/x86/asm.h>
#include <oskit/x86/paging.h>
#include <threads/pthread_internal.h>
#include <threads/x86/pcb.h>

	.text
	.align 8
	.globl EXT(thread_switch_usermode)
LEXT(thread_switch_usermode)
	/* switch to new process. first, save context as needed */
	movl    12(%esp),%ecx			/* Current thread */

	movl	THREAD_PPCB(%ecx),%ecx
	movl	(%esp),%eax			/* Hardware registers */
	movl	%eax,PCB_EIP(%ecx)
	movl	%ebx,PCB_EBX(%ecx)
	movl	%esp,PCB_ESP(%ecx)
	movl	%ebp,PCB_EBP(%ecx)
	movl	%esi,PCB_ESI(%ecx)
	movl	%edi,PCB_EDI(%ecx)
	fsave   PCB_FSTATE(%ecx)

	/*
	 * Save is done, switch to new thread.
	 */
	movl	4(%esp),%ecx			/* New thread */
	movl    %esp,%eax			/* Save stack pointer */

	/* restore context */
	movl	THREAD_PPCB(%ecx),%edx
	frstor  PCB_FSTATE(%edx)
	movl	PCB_ESP(%edx),%esp
	movl	PCB_EBP(%edx),%ebp
	movl	PCB_ESI(%edx),%esi
	movl	PCB_EDI(%edx),%edi
	movl	PCB_EIP(%edx),%ebx
	movl	%ebx,(%esp)
	movl	PCB_EBX(%edx),%ebx

	/*
	 * Unlock the lock
	 */
	movl	8(%eax),%eax			/* Lock to unlock */
        xorl    %ecx,%ecx
        xchgl   %ecx,(%eax)

	movl	$0,%eax
	ret

	.align 8
	.globl EXT(thread_switch_realmode)
LEXT(thread_switch_realmode)
	/* switch to new process. first, save context as needed */
	movl    12(%esp),%ecx			/* Current Thread */

	movl	THREAD_PPCB(%ecx),%ecx
	movl	(%esp),%eax			/* Hardware registers */
	movl	%eax,PCB_EIP(%ecx)
	movl	%ebx,PCB_EBX(%ecx)
	movl	%esp,PCB_ESP(%ecx)
	movl	%ebp,PCB_EBP(%ecx)
	movl	%esi,PCB_ESI(%ecx)
	movl	%edi,PCB_EDI(%ecx)
	movl	%cr2,%eax
	movl	%eax,PCB_CR2(%ecx)
#ifdef  SMP
	fsave   PCB_FSTATE(%ecx)
#endif

	/*
	 * Save is done, switch to new thread.
	 */
	movl	4(%esp),%ecx			/* New thread */
	movl    %esp,%eax			/* Save stack pointer */

	/* restore context */
	movl	THREAD_PPCB(%ecx),%edx
#ifdef  SMP
	frstor  PCB_FSTATE(%edx)
#endif
	movl	PCB_ESP(%edx),%esp
	movl	PCB_EBP(%edx),%ebp
	movl	PCB_ESI(%edx),%esi
	movl	PCB_EDI(%edx),%edi
	movl	PCB_CR2(%edx),%ebx
	movl	%ebx,%cr2
	movl	PCB_EIP(%edx),%ebx
	movl	%ebx,(%esp)
	movl	PCB_EBX(%edx),%ebx

#ifndef SMP
	/*
	 * The floating point is disabled if the thread being switched into
	 * is not the current owner of the FPU.
	 */
	clts
	cmpl	%ecx,EXT(owner_fpu)
	je      skipts
	mov	%cr0,%edx
	orl	$0x8,%edx
	mov	%edx,%cr0
skipts:
#endif
	/*
	 * Unlock the lock
	 */
	movl	8(%eax),%eax			/* Lock to unlock */
        xorl    %ecx,%ecx
        xchgl   %ecx,(%eax)

	movl	$0,%eax
	ret

#ifndef SMP
/*
 * FPU fault handler. Switch the FPU context by saving the state off
 * and restoring the current threads state.
 */
	.align 8
	.globl EXT(pthread_fpu_trap_handler)
LEXT(pthread_fpu_trap_handler)
	/*
	 * Turn on access to FP unit.
	 */
	clts

	/*
	 * Get the owner thread of the floating point unit and save to it.
	 */
	movl	EXT(owner_fpu),%ecx
	movl	THREAD_PPCB(%ecx),%edx
	fsave   PCB_FSTATE(%edx)

	/*
	 * Get the current thread and restore the floating point unut.
	 */
	movl	EXT(threads_curthreads),%ecx
	movl	THREAD_PPCB(%ecx),%edx
	frstor  PCB_FSTATE(%edx)
	movl	%ecx,EXT(owner_fpu)

	movl	$0,%eax
	ret
#endif

/*
 * ffs(value)
 *	finds the first bit set in value and returns the index of 
 *	that bit.  Bits are numbered starting from 1, starting at the
 *	rightmost bit.  A return value of 0 means that the argument
 *	was zero.
 *
 * Written by:
 *	J.T. Conklin (jtc@wimsey.com), Winning Strategies, Inc.
 */
	.align 8
	.globl EXT(ffs)
LEXT(ffs)
	bsfl	4(%esp),%eax
	jz	L1	 		/* ZF is set if all bits are 0 */
	incl	%eax			/* bits numbered from 1, not 0 */
	ret

	.align 2
L1:	xorl	%eax,%eax		/* clear result */
	ret

/*
 * thread_fpuinit(src, dst)
 */
	.align 8
	.globl EXT(thread_fpuinit)
LEXT(thread_fpuinit)
	/*
	 * Save current state.
	 */
	movl	4(%esp),%eax
	fsave	0(%eax)

	/*
	 * Reinit and save to new thread fstate.
	 */
	finit
	movl	8(%esp),%eax
	fsave	0(%eax)

	/*
	 * And restore old state.
	 */
	movl	4(%esp),%eax
	frstor  0(%eax)

	ret
