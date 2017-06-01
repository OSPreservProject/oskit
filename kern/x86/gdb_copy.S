/*
 * Copyright (c) 1994-1996, 1999 Sleepless Software
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * Remote PC serial-line debugging for the Flux OS Toolkit
 */

#include <oskit/x86/asm.h>
#include <oskit/x86/proc_reg.h>

	.text

/*
 * Copy between kernel address space and a GDB buffer,
 * detecting and recovering from any invalid accesses that occur.
 *
 * arg0:	source address
 * arg1:	destination address
 * arg2:	byte count
 */
ENTRY(gdb_copyin)
	pushl	%esi
	pushl	%edi			/* save registers */

	movl	8+S_ARG0,%esi		/* get user start address */
	movl	8+S_ARG1,%edi		/* get kernel destination address */
	movl	8+S_ARG2,%edx		/* get count */

	movl	$copy_fail,EXT(gdb_trap_recover)

	/*cld*/				/* count up: default in all GCC code */
	movl	%edx,%ecx		/* move by longwords first */
	shrl	$2,%ecx
	rep
	movsl				/* move longwords */
	movl	%edx,%ecx		/* now move remaining bytes */
	andl	$3,%ecx
	rep
	movsb

	xorl	%eax,%eax		/* return 0 for success */

copy_ret:
	movl	$0,EXT(gdb_trap_recover)

	popl	%edi			/* restore registers */
	popl	%esi
	ret				/* and return */

copy_fail:
	movl	$-1,%eax		/* return -1 for failure */
	jmp	copy_ret		/* pop frame and return */

/*
 * Writing from the gdb stub's buffer into kernel address space
 * can be nearly the same as copying the other direction.
 * But in a kernel that uses the processor's WP flag to catch writes
 * into kernel text, we still want the gdb stub to be able to write
 * breakpoints into the text, so we temporarily disable the WP flag.
 */
ENTRY(gdb_copyout)
	movl	%cr0,%eax
	testl	$CR0_WP,%eax
	je	EXT(gdb_copyin)
	xorl	$CR0_WP,%eax
	movl	%eax,%cr0
       	pushl	12(%esp)
       	pushl	12(%esp)
       	pushl	12(%esp)
	call	EXT(gdb_copyin)
	addl	$12,%esp
	movl	%cr0,%ecx
	orl	$CR0_WP,%ecx
	movl	%ecx,%cr0
	ret
