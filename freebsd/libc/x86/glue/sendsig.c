/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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

#include <oskit/x86/base_trap.h>
#include <oskit/x86/trap.h>
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
#ifdef KNIT
        oskit_sendsig_init(machdep_deliver_signal);
#else
	/* This will produce a warning. */
	oskit_libcenv_signals_init(libc_environment, machdep_deliver_signal);
#endif
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
	sc.sc_esp = ts->esp;
	sc.sc_ebp = ts->ebp;
	sc.sc_isp = 0;		/* What is isp? */
	sc.sc_eip = ts->eip;
	sc.sc_efl = ts->eflags;
	sc.sc_es  = ts->es;
	sc.sc_ds  = ts->ds;
	sc.sc_cs  = ts->cs;
	sc.sc_ss  = ts->ss;
	sc.sc_edi = ts->edi;
	sc.sc_esi = ts->esi;
	sc.sc_ebx = ts->ebx;
	sc.sc_edx = ts->edx;
	sc.sc_ecx = ts->ecx;
	sc.sc_eax = ts->eax;
	sc.sc_trapno = ts->trapno;
	sc.sc_err    = ts->cr2;	/* Gleaned from kernel code */

	if (signo == SIGFPE)
		/* XXX translate to canonical BSD codes from
		   from sys/i386/include/trap.h (aka <machine/trap.h>) */
		switch (code) {
		case T_DIVIDE_ERROR:	code = FPE_INTDIV_TRAP; break;
		case T_OVERFLOW:	code = FPE_INTOVF_TRAP; break;
		case T_OUT_OF_BOUNDS:	code = FPE_SUBRNG_TRAP; break;
		}

	/*
	 * Call into the C library ...
	 */
	oskit_libc_sendsig(signo, code, &sc);

	/*
	 * Restore the machine state from the sigcontext, thus allowing
	 * the application to do whatever it likes.
	 */
	ts->esp  = sc.sc_esp;
	ts->ebp  = sc.sc_ebp;
	ts->eip  = sc.sc_eip;
	ts->eflags = sc.sc_efl;
	ts->es   = sc.sc_es;
	ts->ds   = sc.sc_ds;
	ts->cs   = sc.sc_cs;
	ts->ss   = sc.sc_ss;
	ts->edi  = sc.sc_edi;
	ts->esi  = sc.sc_esi;
	ts->ebx  = sc.sc_ebx;
	ts->edx  = sc.sc_edx;
	ts->ecx  = sc.sc_ecx;
	ts->eax  = sc.sc_eax;

	return 0;
}
