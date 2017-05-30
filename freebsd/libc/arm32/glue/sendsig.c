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
 * Very primitive signal posting mechanism. The kernel library uses this
 * upcall to pass trap information up, which is converted into a more
 * standard sigcontext structure, and then delivered to the application.
 */

#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/trap.h>
#include <oskit/debug.h>
#include <oskit/c/environment.h>

#include <stdlib.h>
#include <signal.h>

void
oskit_sendsig_init(int (*deliver_function)(int, int, struct trap_state *));

int	machdep_deliver_signal(int signo, int code, struct trap_state *ts);
void	oskit_libc_sendsig(int sig, int code, struct sigcontext *scp);

/*
 * Tell the kernel library about the upcall.
 */
void
libc_sendsig_init()
{
	/* This will produce a warning. */
	oskit_libcenv_signals_init(libc_environment, machdep_deliver_signal);
}

/*
 * External entrypoint for modules that catch particular traps (like svm)
 * and want to turn them into delivered signals.
 */
int
machdep_deliver_signal(int signo, int code, struct trap_state *ts)
{
	struct sigcontext	sc;

	/*
	 * Build a partial sigcontext, filling in the hardware specific
	 * details, but leaving signal specific stuff (like sigmask) to
	 * the actual signal implementation in the C library. This keeps
	 * things sorta independent of each other.
	 */
	sc.sc_spsr      = ts->spsr;
	sc.sc_r0        = ts->r0;
	sc.sc_r1        = ts->r1;
	sc.sc_r2        = ts->r2;
	sc.sc_r3        = ts->r3;
	sc.sc_r4        = ts->r4;
	sc.sc_r5        = ts->r5;
	sc.sc_r6        = ts->r6;
	sc.sc_r7        = ts->r7;
	sc.sc_r8        = ts->r8;
	sc.sc_r9        = ts->r9;
	sc.sc_r10       = ts->r10;
	sc.sc_r11       = ts->r11;
	sc.sc_r12       = ts->r12;
	sc.sc_usr_sp	= ts->svc_sp;	/* XXX No user mode! */
	sc.sc_usr_lr    = ts->svc_lr; 
	sc.sc_svc_lr    = ts->svc_lr;
	sc.sc_pc        = ts->pc;
	
	/*
	 * Call into the C library ...
	 */
	oskit_libc_sendsig(signo, code, &sc);

	/*
	 * Restore the machine state from the sigcontext, thus allowing
	 * the application to do whatever it likes.
	 */
	ts->spsr     = sc.sc_spsr;
	ts->r0       = sc.sc_r0;
	ts->r1       = sc.sc_r1;
	ts->r2       = sc.sc_r2;
	ts->r3       = sc.sc_r3;
	ts->r4       = sc.sc_r4;
	ts->r5       = sc.sc_r5;
	ts->r6       = sc.sc_r6;
	ts->r7       = sc.sc_r7;
	ts->r8       = sc.sc_r8;
	ts->r9       = sc.sc_r9;
	ts->r10      = sc.sc_r10;
	ts->r11      = sc.sc_r11;
	ts->r12      = sc.sc_r12;
	ts->svc_sp   = sc.sc_usr_sp;	/* XXX No user mode! */
	ts->svc_lr   = sc.sc_usr_lr; 
	ts->pc       = sc.sc_pc;

	return 0;
}
