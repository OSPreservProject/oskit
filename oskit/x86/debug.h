/*
 * Copyright (c) 1994-1995, 1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_X86_DEBUG_H_
#define _OSKIT_X86_DEBUG_H_

#define PRINT_STACK_TRACE(num_frames) {			\
	int i = (num_frames);				\
	unsigned *fp;					\
	printf("stack trace:\n");			\
	asm volatile ("movl %%ebp,%0" : "=r" (fp));	\
	while (fp && --i >= 0) {			\
		printf("\t0x%08x\n", *(fp + 1));	\
		fp = (unsigned *)(*fp);			\
	}						\
}

#ifdef ASSEMBLER

/*
 * Simple assert macro for assembly language.
 * The a and b parameters form an instruction, typically a compare,
 * and then the cond is used to branch if the condition is true.
 * For example, 'A(eq,cmpl %eax,%ebx)' asserts that eax is equal to ebx,
 * and will cause an interrupt 0xda otherwise.
 */
#ifdef DEBUG
#define A(cond,a,b)			\
	a,b				;\
	j##cond	8f			;\
	int	$0xda			;\
8:
#else
#define A(cond,a,b)
#endif

#else /* not ASSEMBLER */


/*
 * Version of assert() for 16-bit x86 code.
 * This is identical to the version in oskit/c/assert.h,
 * except for the addition of the i16_ prefix.
 */
#ifdef NDEBUG
#define i16_assert(ignore) ((void)0)
#else

#include <oskit/compiler.h>

OSKIT_BEGIN_DECLS
extern void i16_panic(const char *format, ...) OSKIT_NORETURN;
OSKIT_END_DECLS

#define i16_assert(expression)  \
	((void)((expression) ? 0 : \
		(i16_panic(__FILE__":%u: failed assertion `"#expression"'", \
			   __LINE__), 0)))
#endif


#endif /* not ASSEMBLER */

#endif /* _OSKIT_X86_DEBUG_H_ */
