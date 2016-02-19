/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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

#include <oskit/arm32/trap.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/proc_reg.h>

#include <oskit/debug.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/signal.h>

/* the C library entrypoint for delivering a signal */
static int	(*sendsig_deliver)(int sig, int code, struct trap_state *ts);

/*
 * This should be called by the C library to install the entrypoint
 * to deliver signals to.
 */
void
oskit_sendsig_init(int (*deliver_function)(int, int, struct trap_state *ts))
{
	sendsig_deliver = deliver_function;

	/*
	 * Reset the trap handlers, except for ones that have already
	 * been changed (like GDB handlers).
	 */
#define CHANGETRAP(trap) \
	if (base_trap_handlers[trap] == base_trap_default_handler) \
		base_trap_handlers[trap] = sendsig_trap_handler

	CHANGETRAP(T_RESET);
	CHANGETRAP(T_UNDEF);
	CHANGETRAP(T_SWI);
	CHANGETRAP(T_PREFETCH_ABORT);
	CHANGETRAP(T_DATA_ABORT);
	CHANGETRAP(T_ADDREXC);
	CHANGETRAP(T_IRQ);
	CHANGETRAP(T_FIQ);
	CHANGETRAP(T_STACK_OVERFLOW);

#undef  CHANGETRAP
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
	 * Call into the C library ...
	 */
	sendsig_deliver(signo, ts->trapno, ts);

	return 0;
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
	case T_RESET          : signo = SIGTERM;  break;
	case T_UNDEF          : signo = SIGILL;   break;
	case T_SWI            : signo = SIGEMT;   break;
	case T_PREFETCH_ABORT : signo = SIGSEGV;  break;
	case T_DATA_ABORT     : signo = SIGSEGV;  break;
	case T_ADDREXC        : signo = SIGSEGV;  break;
	case T_IRQ            : signo = SIGINT;   break; /* ??? */
	case T_FIQ            : signo = SIGINT;   break; /* ??? */
	case T_STACK_OVERFLOW : signo = SIGBUS;  break;
	default : panic("sendsig: Illegal trap number: %d", ts->trapno);
        }

	return oskit_sendsig(signo, ts);
}
