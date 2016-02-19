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
 * Copyright (c) 1992,1991,1990,1989 Carnegie Mellon University
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

#include <oskit/x86/asm.h>


#if 0 /* is this useful? */

/* l2rbcopy(from, to, bcount) */

ENTRY(l2rbcopy)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	pushl	%esi
	movl	B_ARG2,%edx
	movl	B_ARG1,%edi
	movl	B_ARG0,%esi
/* move longs */
0:	cld
	movl	%edx,%ecx
	sarl	$2,%ecx
	js	1f
	rep
	movsl
/* move bytes */
	movl	%edx,%ecx
	andl	$3,%ecx
	rep
	movsb
1:
	popl	%esi
	popl	%edi
	leave
	ret	
#endif



/* 
 * bcopy - like l2rbcopy, but recognizes overlapping ranges and handles 
 *           them correctly.
 *	  bcopy(from, to, bytes)
 *		char *from, *to;
 *		int bytes;
 */
ENTRY(bcopy)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	pushl	%esi
	movl	B_ARG0,%esi
	movl	B_ARG1,%edi
bcopy_common:
	movl	B_ARG2,%edx
	leal	(%esi,%edx), %eax	/* from + bytes */
	cmpl	%eax, %edi		/* <= to 	*/
	jae	0f
	leal	(%edi,%edx), %eax	/* to + bytes	*/
	cmpl	%eax, %esi		/* <= from	*/
	jae	0f
	cmpl	%esi, %edi		/* from > to	*/
	jb	0f
	je 	1f

	addl	%edx, %esi; decl %esi
	addl	%edx, %edi; decl %edi
	std
	movl	%edx,%ecx
/* move bytes backwards */
	rep
	movsb
	cld
1:
/* memcpy: return dest pointer */
	movl	B_ARG0,%eax
	popl	%esi
	popl	%edi
	leave
	ret	
/* move words forwards */
0:	cld
	movl	%edx,%ecx
	sarl	$2,%ecx
	js	2f
	rep
	movsl
/* move bytes forwards */
	movl	%edx,%ecx
	andl	$3,%ecx
	rep
	movsb
2:
/* memcpy: return dest pointer */
	movl	B_ARG0,%eax
	popl	%esi
	popl	%edi
	leave
	ret	

/* memcpy(to, from, count) */

/* 
 * memmove is just memcpy that deals with overlapping regions.  Since
 * our memcpy does that anyway, it's just an alternate entry point for 
 * it.
 */
ENTRY2(memcpy, memmove)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	pushl	%esi
	movl	B_ARG0,%edi
	movl	B_ARG1,%esi
	jmp	bcopy_common


#ifdef __ELF__
	/*
	 * Since this file defines both bcopy and memcpy, a file in
	 * another library defining just bcopy will conflict with
	 * this file when linked in later to define memcpy (or vice versa).
	 * In ELF, we can avoid this probably easily using weak symbols.
	 */
	.weak bcopy
	.weak memcpy
	.weak memmove
#endif

