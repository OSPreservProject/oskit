/*
 * Copyright (c) 1997, 1998, 1999, 2000 University of Utah and the Flux Group.
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
 * this is a completely ad-hoc signal handling facility,
 *
 * compliant to pretty much nothing. 
 */
#include <oskit/c/errno.h>
#include <sys/types.h>
#include <signal.h>
#include <strings.h>
#include <stdlib.h>

#ifdef THREAD_SAFE
/*
 * Thread safe implementation is with the threads library.
 */
#else
static sigset_t		sigmask;
static struct sigaction sigactions[NSIG];
static sigset_t		sigpend;
#ifndef KNIT
static int		initialized;
#endif

/* This handles the details of posting the signal to the application. */
void	really_deliver_signal(int sig, siginfo_t *code,struct sigcontext *scp);

/*
 * Init the signal code. Must be called from oskit_init_libc if the
 * application wants to properly handle signals.
 */
void
signals_init(void)
{
	int	i;

	/* Initialize the default signal actions. */
	for (i = 0; i < NSIG; i++)
		sigactions[i].sa_handler = SIG_DFL;

	libc_sendsig_init();

#ifndef KNIT
	initialized = 1;
#endif
}

/*
 * sigaction
 */
int
sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
#ifndef KNIT
	if (!initialized)
		signals_init();
#endif
	
	if (sig < 0 || sig >= NSIG)
		return errno = EINVAL, -1;

	if (oact)
		*oact = sigactions[sig];
	if (act)
		sigactions[sig] = *act;

	return 0;
}

/*
 * sigprocmask
 */
int
sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
#ifndef KNIT
	if (!initialized)
		signals_init();
#endif
	
	if (oset)
		*oset = sigmask;

	if (set) {
		switch (how) {
		case SIG_BLOCK:
			sigmask |= *set;
			break;
		case SIG_UNBLOCK:
			sigmask &= ~*set;
			break;
		case SIG_SETMASK:
			sigmask = *set;
			break;
		default:
			errno = EINVAL;
			return -1;
		}
	}

	/*
	 * Look for pending signals that are now unblocked, and deliver.
	 */
	while (sigpend & ~sigmask) {
		int sig = ffs(sigpend & ~sigmask);

		raise(sig);
	}
	return 0;
}

/*
 * raise a signal. Note that taking the siglock here is actually bogus since
 * raise is called out of upcalls (setitimer) from the kernel library. If
 * the lock to held, all is lost.
 */
int
raise(int sig)
{
	struct sigcontext	sc;
	siginfo_t		siginfo;

#ifndef KNIT
	if (!initialized)
		signals_init();
#endif
	
	if (sig < 0 || sig >= NSIG)
		return errno = EINVAL, -1;

	sigaddset(&sigpend, sig);
	if (sigismember(&sigmask, sig))
		return 0;

	/* create a stub sigcontext_t. */
	bzero(&sc, sizeof(sc));

	siginfo.si_signo           = sig;
	siginfo.si_code            = SI_USER;
	siginfo.si_value.sival_int = 0;

	really_deliver_signal(sig, &siginfo, &sc);

	return 0;
}

/*
 * Kill is the process equivalent, which in the single-threaded oskit
 * can be mapped to raise, since there is only one process/thread.
 */
int
kill(pid_t pid, int sig)
{
	return raise(sig);
}

/*
 * Deliver a signal generated from a trap. This is the upcall from the
 * machine dependent code that created the sigcontext structure from
 * the trap state. Note that interrupt type signals are not generated this
 * way, and is really the reason this stuff is bogus.
 */
void
oskit_libc_sendsig(int sig, int code, struct sigcontext *scp)
{
	siginfo_t		siginfo;

	/*
	 * Look at the sigmask. If the signal is blocked, it means
	 * deadlock since there will be no way for the signal to be
	 * unblocked. Thats a feature of this really dumb implementation
	 */
	if (sigismember(&sigmask, sig)) {
		panic("deliver_signal: "
		      "Signal %d (an exception) is blocked", sig);
	}

	siginfo.si_signo           = sig;
	siginfo.si_code            = SI_EXCEP;
	siginfo.si_value.sival_int = code;

	really_deliver_signal(sig, &siginfo, scp);
}

/*
 * This is called with the signal lock taken so that mask and the
 * sigacts structure is protected until the handler is actually
 * called. The lock is released before calling the handler, and control
 * returns to the caller with the lock unlocked. This is not the most
 * ideal way to do this, but it needs to be simple right now.
 */
void
really_deliver_signal(int sig, siginfo_t *info, struct sigcontext *scp)
{
	sigset_t		oldmask;
	struct sigaction	*act;

	act = &sigactions[sig];

	if (act->sa_handler == SIG_IGN || act->sa_handler == SIG_ERR)
		return;

	if (act->sa_handler == SIG_DFL) {
		/* Default action for all signals is termination */
		panic("libc sendsig: Signal %d caught but no handler", sig);
	}

	oldmask = sigmask;
	sigaddset(&sigmask, sig);
	sigmask |= sigactions[sig].sa_mask;
	sigdelset(&sigpend, sig);

	/* call the handler */
	if (sigactions[sig].sa_flags & SA_SIGINFO)
		sigactions[sig].sa_sigaction(sig, info, (void *) scp);
	else
		((void (*)(int, int, struct sigcontext *))
		 sigactions[sig].sa_handler)(sig,
					     info->si_value.sival_int, scp);

	sigmask = oldmask;
}

/*
 * Sigwait.
 */
int
sigwait(const sigset_t *set, int *sig)
{
	return ENOSYS;
}

/*
 * Sigwaitinfo. 
 */
int
sigwaitinfo(const sigset_t *set, siginfo_t *info)
{
	return ENOSYS;
}

/*
 * Sigtimedwait.
 */
int
sigtimedwait(const sigset_t *set,
	     siginfo_t *info, const struct oskit_timespec *timeout)
{
	return ENOSYS;
}

int
sigsuspend(const sigset_t *sigmask)
{
	return ENOSYS;
}
#endif /* THREAD_SAFE */
