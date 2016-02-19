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
 * This file contains the arm-specific code for remote GDB debugging.
 * When a trap occurs, we take a standard trap saved state frame
 * (defined in oskit/arm32/base_trap.h),
 * and convert it into the gdb_state structure that GDB wants.
 * We also convert the arm32 trap number into an appropriate signal number.
 *
 * This code can interface with different remote GDB stubs,
 * e.g. for the serial versus Ethernet protocols.
 * The 'gdb_stub' function pointer is used to call the correct stub.
 */

#include <signal.h>
#include <oskit/arm32/trap.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/proc_reg.h>
#include <oskit/arm32/gdb.h>
#include <oskit/c/stdlib.h>

void (*gdb_signal)(int *inout_signo, struct gdb_state *inout_state);

unsigned gdb_trap_recover;

int gdb_trap(struct trap_state *ts)
{
	struct gdb_state r;
	int signo, orig_signo;
	oskit_u32_t pc;

	if (gdb_signal == 0)
		return -1;

	/* If a recovery address has been set,
	   use it and return immediately.  */
	if (gdb_trap_recover)
	{
		ts->pc = gdb_trap_recover;
		return 0;
	}
	pc = ts->pc;

	/* Convert the arm32 trap code into a generic signal number.  */
	/* XXX some of these are probably not really right.  */
	switch (ts->trapno)
	{
#define TRAP(trap, sig, adjust) case trap: signo = sig; pc += adjust; break
		TRAP(-1,		SIGINT,   0);
		TRAP(T_SWI,		SIGEMT,  -4);
		TRAP(T_PREFETCH_ABORT,	SIGSEGV, -4);
		TRAP(T_DATA_ABORT,	SIGSEGV, -8);
		TRAP(T_ADDREXC,		SIGSEGV, -4);
		TRAP(T_FIQ,		SIGTRAP, -4);
		TRAP(T_STACK_OVERFLOW,	SIGBUS,  -8);
	case T_UNDEF:
		/*
		 * Check for GDB breakpoint
		 */
		if (*((unsigned *) (ts->pc - 4)) == 0xe7ffdefe) {
			signo = SIGTRAP;
			pc -= 4;
		}
		else if (*((unsigned *) (ts->pc - 4)) == 0xe7ffdefd) {
			/* See gdb_set_bkpt() */
			signo = SIGTRAP;
		}
		else {
			signo = SIGILL;
			pc -= 4;
		}
		break;
	default:
		signo = SIGEMT;         /* "software generated"*/
#undef	TRAP
	}
	orig_signo = signo;

	/* Convert the trap state into GDB's format.  */
	memset((void *) &r, 0, sizeof(r));
	r.r0   = ts->r0;
	r.r1   = ts->r1;
	r.r2   = ts->r2;
	r.r3   = ts->r3;
	r.r4   = ts->r4;
	r.r5   = ts->r5;
	r.r6   = ts->r6;
	r.r7   = ts->r7;
	r.r8   = ts->r8;
	r.r9   = ts->r9;
	r.r10  = ts->r10;
	r.r11  = ts->r11;
	r.r12  = ts->r12;
	r.ps   = ts->spsr;
	r.pc   = pc;

	/*
	 * XXX Not bothering with user mode stuff
	 */
	r.sp   = ts->svc_sp;
	r.lr   = ts->svc_lr;

	/* Call the appropriate GDB stub to do its thing.  */
	gdb_signal(&signo, &r);

	/* Stuff GDB's modified state into our trap_state.  */
	ts->r0     = r.r0;
	ts->r1     = r.r1;
	ts->r2     = r.r2;
	ts->r3     = r.r3;
	ts->r4     = r.r4;
	ts->r5     = r.r5;
	ts->r6     = r.r6;
	ts->r7     = r.r7;
	ts->r8     = r.r8;
	ts->r9     = r.r9;
	ts->r10    = r.r10;
	ts->r11    = r.r11;
	ts->r12    = r.r12;
	ts->pc     = r.pc;
	ts->svc_lr = r.lr;

	/*
	 * XXX Not bothering with user mode stuff
	 */
	{
		/* XXX currently we don't know how to change the kernel esp.
		   We could do it by physically moving the trap frame.  */
		if (r.sp != (unsigned)ts->svc_sp)
			panic("gdb_trap: can't change stack pointer");
	}

	if (r.ps != ts->spsr)
		panic("gdb_trap: can't change PSR: 0x%x 0x%x", r.ps, ts->spsr);

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
			case SIGTRAP:	ts->trapno = T_UNDEF; break;
			case SIGILL:	ts->trapno = T_UNDEF; break;
			case SIGSEGV:	ts->trapno = T_DATA_ABORT;
					break;
		}

		return -1;
	}

	/* GDB consumed the signal - just resume execution normally.  */
	return 0;
}
