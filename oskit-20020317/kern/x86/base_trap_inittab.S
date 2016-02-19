/*
 * Copyright (c) 1995-1999 University of Utah and the Flux Group.
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
	jmp	alltraps

/*
 * User-accessible exception.  Otherwise, same as above.
 */
#define	EXCEP_USR(n,name)					\
	GATE_ENTRY(n,name,ACC_PL_U | ACC_TRAP_GATE)		;\
name:								;\
	pushl	$(0)						;\
	pushl	$(n)						;\
	jmp	alltraps

/*
 * Error code has been pushed.  Just push trap number.
 */
#define	EXCEP_ERR(n,name)					\
	GATE_ENTRY(n,name,ACC_PL_K | ACC_TRAP_GATE)		;\
name:								;\
	pushl	$(n)						;\
	jmp	alltraps


GATE_INITTAB_BEGIN(base_trap_inittab)

EXCEPTION(0x00,t_zero_div)
EXCEPTION(0x01,t_debug)
EXCEP_USR(0x03,t_int3)
EXCEP_USR(0x04,t_into)
EXCEP_USR(0x05,t_bounds)
EXCEPTION(0x06,t_invop)
EXCEPTION(0x07,t_nofpu)
EXCEPTION(0x08,a_dbl_fault)
EXCEPTION(0x09,a_fpu_over)
EXCEP_ERR(0x0a,a_inv_tss)
EXCEP_ERR(0x0b,t_segnp)
EXCEP_ERR(0x0c,t_stack_fault)
EXCEP_ERR(0x0d,t_gen_prot)
EXCEP_ERR(0x0e,t_page_fault)
EXCEPTION(0x0f,t_trap_0f)
EXCEPTION(0x10,t_fpu_err)
EXCEPTION(0x11,t_trap_11)
EXCEPTION(0x12,t_trap_12)
EXCEPTION(0x13,t_trap_13)
EXCEPTION(0x14,t_trap_14)
EXCEPTION(0x15,t_trap_15)
EXCEPTION(0x16,t_trap_16)
EXCEPTION(0x17,t_trap_17)
EXCEPTION(0x18,t_trap_18)
EXCEPTION(0x19,t_trap_19)
EXCEPTION(0x1a,t_trap_1a)
EXCEPTION(0x1b,t_trap_1b)
EXCEPTION(0x1c,t_trap_1c)
EXCEPTION(0x1d,t_trap_1d)
EXCEPTION(0x1e,t_trap_1e)
EXCEPTION(0x1f,t_trap_1f)

GATE_INITTAB_END

alltraps:

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

#ifdef __ELF__	/* XXX only work for ELF right now */
	/*
	 * If supervisor mode, check for trap annotations.
	 * If the table is not empty and an annotation exists for this EIP,
	 * call the registered handler in place of the standard handler.
	 */
	testb	$3,TS_CS(%esp)
	jnz	1f
	cmpl	$0,EXT(anno_trap)
	jz	1f
	pushl	TS_EIP(%esp)
	pushl	$EXT(anno_trap)
	call	EXT(anno_find_exact)
	addl	$8,%esp
	orl	%eax,%eax
	jz	1f
	pushl	%esp
	pushl	%eax
	movl	4(%eax),%eax
	call	*%eax
	addl	$8,%esp
	jmp	2f
#endif /* __ELF__ */

1:
	.comm	EXT(base_trap_gdb_handler),4
	movl	EXT(base_trap_gdb_handler),%eax
	orl	%eax,%eax
	jnz	4f

3:
	/* Call the C handler function if one has been installed.  */
	movl	TS_TNO(%esp),%eax
	movl	EXT(base_trap_handlers)(,%eax,4),%eax
	orl	%eax,%eax
	jz	1f
#ifdef FULL_STACK_TRACE
	/*
	 * Fake a stack frame for back traces
	 */
	movl	%esp,%edx
	pushl	TS_EIP(%esp)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edx
	call	*%eax
	movl	%ebp,%esp
	popl	%ebp
	popl	%edx
#else
	pushl	%esp
	call	*%eax
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

4:
	/*
	 * If the gdb handler function returned zero (success),
	 * then resume execution as if the trap never happened.
	 * Otherwise, go back and check for a normal C handler.
	 */
	pushl	%esp
	call	*%eax
	popl	%edx
	orl	%eax,%eax
	jz	2b	/* gdb handler happy, resume above */
	jmp	3b	/* gdb handler not happy, try normal handler */
