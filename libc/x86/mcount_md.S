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
 * Copyright (c) 1996 Bruce D. Evans.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      $\Id: prof_machdep.c,v 1.2 1996/04/08 16:41:06 wollman Exp $
 */

/*
 * mcount and __mcount for the x86.
 */

#if 0
#include "config_profiling.h"
#endif
#define CONFIG_PROFILING
#ifdef CONFIG_PROFILING
			
#include <oskit/x86/asm.h>
#include <oskit/c/sys/gmon.h>

	
	.text

NON_GPROF_ENTRY(__mcount)
        /* Check that we are profiling.  Do it early for speed. */
	cmpl	$GMON_PROF_ON, EXT(_gmonparam) + GM_STATE
	jne	mcount_exit

        /*
	 * __mcount is the same as mcount except the caller hasn't changed
         * the stack except to call here, so the caller's raddr is above
         * our raddr.
	 */
	movl	4(%esp), %edx
	jmp	got_frompc

#ifdef __ELF__	
NON_GPROF_ENTRY(.mcount)
#endif	
NON_GPROF_ENTRY(mcount)
	cmpl	$GMON_PROF_ON, EXT(_gmonparam) + GM_STATE
	jne	mcount_exit

        /*
	 * The caller's stack frame has already been built, so %ebp is
         * the caller's frame pointer.  The caller's raddr is in the
         * caller's frame following the caller's caller's frame pointer.
	 */
	movl	4(%ebp), %edx
	
got_frompc:
	/* Our raddr is the caller's pc. */
	movl	(%esp), %eax

	pushf
	pushl	%eax	/* selfpc */
	pushl	%edx	/* frompc */
#if 0
	cli
#endif
	call	EXT(_mcount)
	addl	$8, %esp
	popf
mcount_exit:
	ret

#endif	/* CONFIG_PROFILING */
