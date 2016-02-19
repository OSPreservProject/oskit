/*
 * Copyright (c) 1994-1996 Sleepless Software
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
#include <oskit/x86/base_trap.h>
	
	.text

ENTRY(gdb_pc_com_intr)

	/* Push a dummy error code and trap number,
	   signaling gdb_trap() that a hardware interrupt occurred.  */
	pushl	$0
	pushl	$-1

	/* Save the rest of the state frame.  */
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	/* Load the kernel's segment registers.  */
	movw	%ss,%ax
	movw	%ax,%ds
	movw	%ax,%es

	/* GCC likes the direction flag cleared.  */
	cld

        /* Acknowledge the interrupt at both the master and the slave */
        movb    $0x20,%al
        outb    %al,$0x20
        outb    %al,$0xa0
	
	pushl	%esp
	call	EXT(gdb_trap_ss)
	popl	%edx

	/*
         * If the handler function returned zero (success),
         * then resume execution as if the trap never happened.
         * Otherwise, just panic.
         */
        orl     %eax,%eax
        jnz     1f

        popl    %gs
        popl    %fs
        popl    %es
        popl    %ds
        popa
        addl    $4*2,%esp       /* Pop trap number and error code */
        iret

1:
        /* Dump the register state and panic.  */
        UNEXPECTED_TRAP
