/*
 * Copyright (c) 1996, 1998, 2001 University of Utah and the Flux Group.
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
 * Very primitive signal support. This allows the C library to register
 * an upcall that is used to deliver traps. The C library is responsible
 * for converting the trap state into a sigcontext and doing everything
 * else. Since the trap state is used to restore state, the application
 * code is free to change the machine state.
 */

#include <oskit/x86/trap.h>
#include <oskit/x86/eflags.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/base_tss.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/debug_reg.h>

#include <oskit/debug.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/signal.h>

/* the C library entrypoint for delivering a signal */
static int	(*sendsig_deliver)(int sig, int code, struct trap_state *ts);

/* the signal sending hook */
typedef int	(*sendsig_hook_t)(int sig, int code, struct trap_state *ts);
static sendsig_hook_t	oskit_sendsig_hook;

/*
 * This should be called by the C library to install the entrypoint
 * to deliver signals to.
 */
void
oskit_sendsig_init(int (*deliver_function)(int, int, struct trap_state *ts))
{
	int	i;
		
	sendsig_deliver = deliver_function;

	/*
	 * Reset the trap handlers, except for ones that have already
	 * been changed (like GDB handlers).
	 */
	for (i = 0; i < BASE_TRAP_COUNT; i++) {
		if (base_trap_handlers[i] == base_trap_default_handler)
			base_trap_handlers[i] = sendsig_trap_handler;
	}

	/*
	 * Set up the debug registers to catch null pointer references.
	 */
	set_b0(NULL, DR7_LEN_1, DR7_RW_INST);
	set_b1(NULL, DR7_LEN_4, DR7_RW_DATA);
}

/*
 * External entrypoint for modules that catch particular traps (like svm)
 * and want to turn them into delivered signals. The signo is provided to
 * the C library as a "hint", but it can do whatever it wants ...
 */
int
oskit_sendsig(int signo, struct trap_state *ts)
{

	/*
	 * Ignore if C library is not interested. Return error code to
	 * ensure that the trap will generate a dump/panic.
	 */
	if (! sendsig_deliver)
		return 1;
	
	/*
	 * Call the signal sendding hook. 
	 */
	if (oskit_sendsig_hook) {
	    if ((*oskit_sendsig_hook)(signo, ts->trapno, ts) == 0) {
		/* If the function returns 0, we don't deliver the signal */
		return 0;
	    }
	}

	/*
	 * Call into the C library ...
	 */
	sendsig_deliver(signo, ts->trapno, ts);

	return 0;
}

/*
 * Install the signal sending hook.  Only one hook can be set.
 * Returns previous hook function.
 */
sendsig_hook_t
oskit_sendsig_hook_set(sendsig_hook_t hook)
{
    sendsig_hook_t ohook = oskit_sendsig_hook;
    oskit_sendsig_hook = hook;
    return ohook;
}

/*
 * Generic trap handler for converting any trap into a signal. Can
 * be called by external modules that do not know what to do with a
 * trap.
 */
int
sendsig_trap_handler(struct trap_state *ts)
{
	int		signo = 0;

	/*
	 * It probably makes no sense to pass all these signals to the
	 * application program.
	 */
        switch (ts->trapno) {
	case -1 : signo = SIGINT;  break;	/* hardware interrupt */
	case 0  : signo = SIGFPE;  break;	/* divide by zero */
	case 1  : signo = SIGTRAP; break;	/* debug exception */
	case 3  : signo = SIGTRAP; break;	/* breakpoint */
	case 4  : signo = SIGFPE;  break;	/* overflow */
	case 5  : signo = SIGFPE;  break;	/* bound instruction */
	case 6  : signo = SIGILL;  break;	/* Invalid opcode */
	case 7  : signo = SIGFPE;  break;	/* coprocessor not available */
	case 9  : signo = SIGSEGV; break;	/* coproc segment overrun*/
	case 10 : signo = SIGSEGV; break;	/* Invalid TSS */
	case 11 : signo = SIGSEGV; break;	/* Segment not present */
	case 12 : signo = SIGSEGV; break;	/* stack exception */
	case 13 : signo = SIGSEGV; break;	/* general protection */
	case 14 : signo = SIGSEGV; break;	/* page fault */
	case 16 : signo = SIGFPE;  break;	/* coprocessor error */
	default : panic("sendsig: Illegal trap number: %d", ts->trapno);
        }

	/*
	 * Look for null pointer exceptions ...
	 */
	if (ts->trapno == 1) {
		unsigned dr6 = get_dr6();
		
		if (dr6 & (DR6_B0|DR6_B1))
			signo = SIGSEGV;
		else
			signo = SIGTRAP;
	}

	return oskit_sendsig(signo, ts);
}
