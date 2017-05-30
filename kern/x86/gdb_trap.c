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
 * This file contains the x86-specific code for remote GDB debugging.
 * When a trap occurs, we take a standard trap saved state frame
 * (defined in oskit/x86/base_trap.h),
 * and convert it into the gdb_state structure that GDB wants.
 * We also convert the x86 trap number into an appropriate signal number.
 *
 * This code can interface with different remote GDB stubs,
 * e.g. for the serial versus Ethernet protocols.
 * The 'gdb_stub' function pointer is used to call the correct stub.
 */

#include <signal.h>
#include <oskit/x86/trap.h>
#include <oskit/x86/eflags.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/gdb.h>
#include <oskit/c/stdlib.h>

void (*gdb_signal)(int *inout_signo, struct gdb_state *inout_state);

unsigned gdb_trap_recover;
unsigned gdb_trap_mask;

int gdb_trap(struct trap_state *ts)
{
	struct gdb_state r;
	int signo, orig_signo;

	if (gdb_signal == 0)
		return -1;

	/* If a recovery address has been set,
	   use it and return immediately.  */
	if (gdb_trap_recover)
	{
		ts->eip = gdb_trap_recover;
		return 0;
	}

	/* Ignore masked hardware interrupts. */
	if (0 <= ts->trapno && ts->trapno <= 31
	    && (gdb_trap_mask & (1 << ts->trapno))) {
	    return -1;
	}

	/* Convert the x86 trap code into a generic signal number.  */
	/* XXX some of these are probably not really right.  */
	switch (ts->trapno)
	{
#ifndef SIGEMT
#define SIGEMT	7		/* XXX */
#endif
#define TRAP(trap, sig) case trap: signo = sig; break
		TRAP(-1,		SIGINT); /* hardware interrupt */
		TRAP(T_DIVIDE_ERROR,	SIGFPE);
	case T_DEBUG:
		signo = SIGTRAP; /* debug exception */
		ts->eflags |= EFL_RF;
		break;
		TRAP(T_INT3,		SIGTRAP); /* int 3 instruction */
		TRAP(T_OVERFLOW,	SIGFPE); /* overflow test */
		TRAP(T_OUT_OF_BOUNDS,	SIGFPE); /* bounds check */
		TRAP(T_INVALID_OPCODE,	SIGILL); /* invalid op code */
		TRAP(T_NO_FPU,		SIGFPE); /* no floating point */
		TRAP(T_DOUBLE_FAULT,	SIGFPE); /* double fault */
		TRAP(T_FPU_FAULT,	SIGEMT);
		TRAP(T_INVALID_TSS,	SIGSEGV);
		TRAP(T_SEGMENT_NOT_PRESENT,SIGSEGV);
		TRAP(T_STACK_FAULT,	SIGSEGV);
		TRAP(T_GENERAL_PROTECTION,SIGSEGV);
		TRAP(T_PAGE_FAULT,	SIGSEGV);
		TRAP(T_FLOATING_POINT_ERROR,SIGFPE);
		TRAP(T_ALIGNMENT_CHECK,	SIGILL);
	default:
		signo = SIGEMT;         /* "software generated"*/
#undef	TRAP
	}
	orig_signo = signo;

	/* Convert the trap state into GDB's format.  */
	r.gs = ts->gs;
	r.fs = ts->fs;
	r.es = ts->es;
	r.ds = ts->ds;
	r.edi = ts->edi;
	r.esi = ts->esi;
	r.ebp = ts->ebp;
	r.ebx = ts->ebx;
	r.edx = ts->edx;
	r.ecx = ts->ecx;
	r.eax = ts->eax;
	r.eip = ts->eip;
	r.cs = ts->cs;
	r.eflags = ts->eflags;
	if ((ts->cs & 3) || (ts->eflags & EFL_VM))
	{
		/* Came from user mode:
		   stack pointer is saved in the trap frame.  */
		r.esp = ts->esp;
		r.ss = ts->ss;
	}
	else
	{
		/* Came from kernel mode: we didn't switch stacks,
		   so stack pointer is ts+TR_KSIZE.  */
		r.esp = (unsigned)&ts->esp;
		r.ss = get_ss();
	}

	/* Call the appropriate GDB stub to do its thing.  */
	gdb_signal(&signo, &r);

	/* Stuff GDB's modified state into our trap_state.  */
	ts->gs = r.gs;
	ts->fs = r.fs;
	ts->es = r.es;
	ts->ds = r.ds;
	ts->edi = r.edi;
	ts->esi = r.esi;
	ts->ebp = r.ebp;
	ts->ebx = r.ebx;
	ts->edx = r.edx;
	ts->ecx = r.ecx;
	ts->eax = r.eax;
	ts->eip = r.eip;
	ts->cs = r.cs;
	ts->eflags = r.eflags;
	if ((r.cs & 3) || (r.eflags & EFL_VM))
	{
		ts->esp = r.esp;
		ts->ss = r.ss;
	}
	else
	{
		/* XXX currently we don't know how to change the kernel esp.
		   We could do it by physically moving the trap frame.  */
		if (r.esp != (unsigned)&ts->esp || r.ss != get_ss())
			panic("gdb_trap: can't change stack pointer");
	}

	/* If GDB sent us back a signal number,
	   convert that back into a trap number.  */
	if (signo != 0)
	{
		/* If the signal number was unchanged from what we sent,
		   leave the trap number unchanged as well.  */
		if (signo == orig_signo)
			return -1;

		/* Otherwise, try to guess an appropriate trap number.
		   We can't do much, but try to get the common ones.
		   If we can't make a decent guess,
		   just leave the trap number unchanged.  */
		switch (signo)
		{
			case SIGFPE:	ts->trapno = T_DIVIDE_ERROR; break;
			case SIGTRAP:	ts->trapno = T_DEBUG; break;
			case SIGILL:	ts->trapno = T_INVALID_OPCODE; break;
			case SIGSEGV:	ts->trapno = T_GENERAL_PROTECTION;
					break;
		}

		return -1;
	}

	/* GDB consumed the signal - just resume execution normally.  */
	return 0;
}
