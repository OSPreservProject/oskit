/*
 * Copyright (c) 1995-1999, 2001 University of Utah and the Flux Group.
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
 * Copyright (c) 1993,1992,1991,1990 Carnegie Mellon University
 * Copyright (c) 1991 IBM Corporation
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation,
 * and that the nema IBM not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written
 * prior permission.
 *
 * CARNEGIE MELLON AND IBM ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON AND IBM DISCLAIM ANY LIABILITY OF ANY KIND FOR
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
/*
 * This file contains a 'gate_init' initialization table
 * to initialize the x86 processor trap vectors to default entrypoints.
 * These entrypoints simply push a standard trap_state frame
 * and jump to the 'trap_handler' routine.
 */

#include <oskit/x86/asm.h>
#include <oskit/x86/eflags.h>
#include <oskit/x86/trap.h>
#include <oskit/x86/seg.h>
#include <oskit/x86/gate_init.h>
#include <oskit/x86/base_trap.h>

#include <oskit/x86/sproc.h>

/*
 * XXX these should either be generated (ala genassym) or defined in
 * base_trap.h.
 */
#define TS_TNO	48
#define TS_EIP	56
#define TS_CS	60

/*
 * No error code.  Clear error code and push trap number.
 */
#define	EXCEPTION(n,name)					\
	GATE_ENTRY(n,name,ACC_PL_K | ACC_TRAP_GATE)		;\
name:								;\
	pushl	$(0)						;\
	pushl	$(n)						;\
	jmp	svc_alltraps

/*
 * User-accessible exception.  Otherwise, same as above.
 */
#define	EXCEP_USR(n,name)					\
	GATE_ENTRY(n,name,ACC_PL_U | ACC_TRAP_GATE)		;\
name:								;\
	pushl	$(0)						;\
	pushl	$(n)						;\
	jmp	svc_alltraps

/*
 * Error code has been pushed.  Just push trap number.
 */
#define	EXCEP_ERR(n,name)					\
	GATE_ENTRY(n,name,ACC_PL_K | ACC_TRAP_GATE)		;\
name:								;\
	pushl	$(n)						;\
	jmp	svc_alltraps

GATE_INITTAB_BEGIN(oskit_sproc_svc_trap_inittab)

EXCEP_USR(SYSCALL_INT,svc_trap)

GATE_INITTAB_END

svc_alltraps:

	/*
	 * Save the rest of the state frame.
	 * Note that on the 486 and up,
	 * pusha is slower than the equivalent set of manual pushes,
	 * so don't do this in your own code if you want speed.
	 * For our purposes here we're more worried
	 * about size and simplicity...
	 */
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

1:

3:
	/* Call the C handler function if one has been installed.  */
#ifdef FULL_STACK_TRACE
	/*
	 * Fake a stack frame for back traces
	 */
	movl	%esp,%edx
	pushl	TS_EIP(%esp)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edx
	call	EXT(oskit_sproc_syscall_handler)
	movl	%ebp,%esp
	popl	%ebp
	popl	%edx
#else
	pushl	%esp
	call	EXT(oskit_sproc_syscall_handler)
	popl	%edx
#endif

2:
	/*
	 * If the handler function returned zero (success),
	 * then resume execution as if the trap never happened.
	 * Otherwise, just panic.
	 */
	orl	%eax,%eax
	jnz	1f

	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popa
	addl	$4*2,%esp	/* Pop trap number and error code */
	iret

1:
	/* Dump the register state and panic.  */
	UNEXPECTED_TRAP
