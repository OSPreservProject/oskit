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
 *	from: @(#)trap.h	5.4 (Berkeley) 5/9/91
 *	trap.h,v 1.3 1993/11/07 17:43:15 wollman Exp
 */

#ifndef _MACHINE_TRAP_H_
#define _MACHINE_TRAP_H_ 1

/* codes for SIGFPE/ARITHTRAP */
#define	    FPE_INTOVF_TRAP	0x1	/* integer overflow */
#define	    FPE_INTDIV_TRAP	0x2	/* integer divide by zero */
#define	    FPE_FLTDIV_TRAP	0x3	/* floating/decimal divide by zero */
#define	    FPE_FLTOVF_TRAP	0x4	/* floating overflow */
#define	    FPE_FLTUND_TRAP	0x5	/* floating underflow */
#define	    FPE_FPU_NP_TRAP	0x6	/* floating point unit not present */
#define	    FPE_SUBRNG_TRAP	0x7	/* subrange out of bounds */

/* codes for SIGBUS */
#define	    BUS_PAGE_FAULT	T_PAGEFLT	/* page fault protection base */
#define	    BUS_SEGNP_FAULT	T_SEGNPFLT	/* segment not present */
#define	    BUS_STK_FAULT	T_STKFLT	/* stack segment */
#define	    BUS_SEGM_FAULT	T_RESERVED	/* segment protection base */

#endif /* _MACHINE_TRAP_H_ */
