/*
 * Copyright (c) 1994-1996 Sleepless Software
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Remote GDB debugging on x86 machines for the Flux OS Toolkit
 * This source file defines a function, gdb_trap_ss(),
 * which is functionally identical to gdb_trap(), except:
 *	- It _must_ be called with the trap_frame
 *	  directly after the return address and state pointer on the stack,
 *	  rather than buried deeper down somewhere.
 *	- If the trap was from supervisor mode,
 *	  it switches and copies the trap frame to a special stack
 *	  so that GDB can safely modify the stack pointer,
 *	  e.g., when calling a function from GDB.
 */

#include <oskit/x86/asm.h>
#include <oskit/x86/eflags.h>

#include "trap_asm.h"

#define GDB_STACK_SIZE 1024

ENTRY(gdb_trap_ss)
	/* XXX check that ts == esp+8 */

	/* If we came from user mode, just use the normal gdb_trap() */
	testl	$EFL_VM,8+TR_EFLAGS(%esp)
	jnz	EXT(gdb_trap)
	testb	$3,8+TR_CS(%esp)
	jnz	EXT(gdb_trap)

	/* There's only one gdb_stack, so we gotta protect it... */
	call	EXT(base_critical_enter)

	/*
	 * Move the saved state frame to the GDB stack,
	 * tricking gdb_trap() into treating this as a ring 1 frame.
	 */
	popl	returnaddr
	popl	%eax	/* throw away trap_state pointer */
	popl	gdb_state+TR_GS
	popl	gdb_state+TR_FS
	popl	gdb_state+TR_ES
	popl	gdb_state+TR_DS
	popl	gdb_state+TR_EDI
	popl	gdb_state+TR_ESI
	popl	gdb_state+TR_EBP
	popl	gdb_state+TR_CR2
	popl	gdb_state+TR_EBX
	popl	gdb_state+TR_EDX
	popl	gdb_state+TR_ECX
	popl	gdb_state+TR_EAX
	popl	gdb_state+TR_TRAPNO
	popl	gdb_state+TR_ERR
	popl	gdb_state+TR_EIP
	popl	%eax			/* Make CS a ring 1 selector */
	orb	$1,%al
	movl	%eax,gdb_state+TR_CS
	popl	gdb_state+TR_EFLAGS
	movl	%esp,gdb_state+TR_ESP
	movw	%ss,%ax			/* Make SS a ring 1 selector */
	orb	$1,%al
	movw	%ax,gdb_state+TR_SS

	/*
	 * Call gdb_trap() on the special gdb_stack
	 */
	movl	$gdb_stack+GDB_STACK_SIZE,%esp
	pushl	$gdb_state
	call	EXT(gdb_trap)

	/* Save gdb_trap's return code in a safe place */
	movl	%eax,%ebx

	/*
	 * Restore the new state frame, possibly in a new location.
	 */
	movl	gdb_state+TR_ESP,%esp
	pushl	gdb_state+TR_EFLAGS
	movl	gdb_state+TR_CS,%eax	/* Change CS back to ring 0 */
	andb	$0xfc,%al
	pushl	%eax
	pushl	gdb_state+TR_EIP
	pushl	gdb_state+TR_ERR
	pushl	gdb_state+TR_TRAPNO
	pushl	gdb_state+TR_EAX
	pushl	gdb_state+TR_ECX
	pushl	gdb_state+TR_EDX
	pushl	gdb_state+TR_EBX
	pushl	gdb_state+TR_CR2
	pushl	gdb_state+TR_EBP
	pushl	gdb_state+TR_ESI
	pushl	gdb_state+TR_EDI
	pushl	gdb_state+TR_DS
	pushl	gdb_state+TR_ES
	pushl	gdb_state+TR_FS
	pushl	gdb_state+TR_GS
	pushl	%esp			/* trap_state parameter */
	pushl	returnaddr

	call	EXT(base_critical_leave)

	/* Return to the trap handler */
	movl	%ebx,%eax
	ret

	.comm	returnaddr,4
	.comm	gdb_state,TR_USIZE
	.comm	gdb_stack,GDB_STACK_SIZE
