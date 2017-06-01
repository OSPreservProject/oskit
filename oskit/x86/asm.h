/*
 * Copyright (c) 1994-1998 University of Utah and the Flux Group.
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
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
#ifndef _OSKIT_X86_ASM_H_
#define _OSKIT_X86_ASM_H_

#include <oskit/config.h>

/* This is a more reliable delay than a few short jmps. */
#define IODELAY	pushl %eax; inb $0x80,%al; inb $0x80,%al; popl %eax

#define S_ARG0	 4(%esp)
#define S_ARG1	 8(%esp)
#define S_ARG2	12(%esp)
#define S_ARG3	16(%esp)

#define FRAME	pushl %ebp; movl %esp, %ebp
#define EMARF	leave

#define B_ARG0	 8(%ebp)
#define B_ARG1	12(%ebp)
#define B_ARG2	16(%ebp)
#define B_ARG3	20(%ebp)

#ifdef __i486__
#define TEXT_ALIGN	4
#else
#define TEXT_ALIGN	2
#endif
#define DATA_ALIGN	2
#define ALIGN		TEXT_ALIGN

/*
 * The .align directive has different meaning on the x86 when using ELF
 * than when using a.out.
 * Newer GNU as remedies this problem by providing .p2align and .balign.
 */
#if defined(HAVE_P2ALIGN)
# define P2ALIGN(p2)	.p2align p2
#elif defined(__ELF__)
# define P2ALIGN(p2)	.align	(1<<(p2))
#else
# define P2ALIGN(p2)	.align	p2
#endif

/* For text alignment, fill in the blank space with nop's (0x90). */
#define ALIGN_TEXT	P2ALIGN(TEXT_ALIGN), 0x90
#define ALIGN_DATA	P2ALIGN(DATA_ALIGN)

#define	LCL(x)	x

#define LB(x,n) n
#ifndef __ELF__
#define EXT(x) _ ## x
#define LEXT(x) _ ## x ## :
#define SEXT(x) "_"#x
#else
#define EXT(x) x
#define LEXT(x) x ## :
#define SEXT(x) #x
#endif
#define GLEXT(x) .globl EXT(x); LEXT(x)
#define LCLL(x) x ## :
#define gLB(n)  n ## :
#define LBb(x,n) n ## b
#define LBf(x,n) n ## f


/* Symbol types */
#ifdef __ELF__
#define FUNCSYM(x)	.type    x,@function
#define DATASYM(x)	.type	 x,@object
#else
#define FUNCSYM(x)	/* nothing */
#define DATASYM(x)	/* nothing */
#endif


#define GEN_ENTRY(x)		ALIGN_TEXT; FUNCSYM(x); GLEXT(x)
#define NON_GPROF_ENTRY(x) 	GEN_ENTRY(x)	

#define	DATA(x)		ALIGN_DATA ; .globl EXT(x) ; DATASYM(x) ; LEXT(x)

/* XXX Does anyone actually _use_ this?  The difference between this and
 * ENTRY isn't anywhere near intuitively obvious.
 */
#define	Entry(x)	GEN_ENTRY(x)

#ifdef GPROF

#define MCOUNT		call EXT(__mcount)
#define	ENTRY(x)	GEN_ENTRY(x) ; MCOUNT 
#define	ENTRY2(x,y)	GEN_ENTRY(y) ; MCOUNT; jmp 2f; \
			GEN_ENTRY(x) ; MCOUNT; 2:
#define	ASENTRY(x) 	.globl x ; FUNCSYM(x); ALIGN_TEXT; gLB(x) ; MCOUNT

#else	/* ndef GPROF */

#define MCOUNT		/* nothing */
#define	ENTRY(x)	GEN_ENTRY(x)
#define	ENTRY2(x,y)	GEN_ENTRY(y); GEN_ENTRY(x)
#define	ASENTRY(x)	.globl x ; FUNCSYM(x); ALIGN_TEXT; gLB(x)

#endif	/* ndef GPROF */


#endif /* _OSKIT_X86_ASM_H_ */
