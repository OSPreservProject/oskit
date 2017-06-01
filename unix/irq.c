/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * "interrupt" support
 */

#include <oskit/dev/dev.h>
#include <oskit/c/environment.h>

#include "native.h"
#include "native_errno.h"
#include "support.h"

#ifndef sigmask
/* This should be in the oskit header file. */
#define sigmask(m)	(1 << ((m)-1))
#endif

/* Signal mask to block. Start with default set */
static sigset_t		sigblocked = { sigmask(SIGALRM) | sigmask(SIGVTALRM) |
					sigmask(SIGIO) };

static sigset_t		irq_sigpending;
static int              irq_enable  = 1;
static int		irq_pending = 0;
static int		sig_blocked = 0;

void (*base_sig_handlers[NSIG])(int, int, struct sigcontext *) = { 0 };

#define BASE_IRQ_SOFTINT_CLEARED		0x80
#define BASE_IRQ_SOFTINT_DISABLED		0x40
unsigned char base_irq_nest = BASE_IRQ_SOFTINT_CLEARED;

static void process_fds();
static void process_signal(int sig, int code, struct sigcontext *scp);

extern void oskit_libc_sendsig(int sig, int code, struct sigcontext *scp);

/*
 * Replace the libc glue code which converts trap_state to
 * sigcontext.  We hope the sigcontext format is the same in the unix
 * kernel as in oskit's libc
 */
void
libc_sendsig_init()
{
	oskit_libcenv_signals_init(libc_environment,
     /* (int(*)(int,int,void*)) */ oskit_libc_sendsig);
}

static void 
sig_handler(int sig, int code, struct sigcontext *scp)
{
	/*
	 * Signals that represent synchronous traps are delivered right
	 * away.  We unblock the trap signal that occured here.
	 * Note that we also unblock the "interrupt" signals if they
	 * were masked just as the result of signal delivery to here
	 * (see oskitunix_set_signal_handler).
	 */
	if (NATIVEOS(sigismember)(&oskitunix_sigtraps, sig)) {
		sigset_t	clear;

		clear = sigblocked;
		NATIVEOS(sigaddset)(&clear, sig);
		NATIVEOS(sigprocmask)(SIG_UNBLOCK, &clear, 0);
		if (!sig_blocked)
			NATIVEOS(sigprocmask)(SIG_UNBLOCK, &sigblocked, 0);

		(*base_sig_handlers[sig])(sig, code, scp);
		return;
	}

	if (!irq_enable) {
		NATIVEOS(sigaddset)(&irq_sigpending, sig);
		irq_pending = 1;
		return;
	}

	process_signal(sig, code, scp);

	if (base_irq_nest == 0) {
		sig_blocked = 1;
		base_irq_nest = BASE_IRQ_SOFTINT_CLEARED |
			BASE_IRQ_SOFTINT_DISABLED;
		base_irq_softint_handler(NULL); /* See NOTE near prototype */
		base_irq_nest &= ~BASE_IRQ_SOFTINT_DISABLED;
	}
}

static void
process_signal(int sig, int code, struct sigcontext *scp)
{
	/* So that machine_intr_enabled returns the right value */
	irq_enable = 0;

	base_irq_nest++;

	if (base_sig_handlers[sig])
		(*base_sig_handlers[sig])(sig, code, scp);
	else {
		printf("sig_handler: irq %d not allocated\n", sig);
		exit(1);
	}

	base_irq_nest--;

	irq_enable = 1;
}

int
oskitunix_set_signal_handler(int sig,
			     void (*handler)(int, int, struct sigcontext *))
{
	struct sigaction sa;

	if (base_sig_handlers[sig]) {
		printf(__FUNCTION__": signal %d already allocated\n", sig);
		exit(1);
	}
	base_sig_handlers[sig] = handler;

	sa.sa_handler = sig_handler;
	sa.sa_flags   = 0;
	sa.sa_mask    = sigblocked;

	if (NATIVEOS(sigaction)(sig, &sa, 0) < 0)
		return -1;

	return 0;
}

static int procmask_count = 0;

static int
procmask(int how)
{
	procmask_count++;
	return NATIVEOS(sigprocmask)(how, &sigblocked, 0);
}

void
osenv_intr_enable()
{
	if (irq_pending) {
		int	sig = 0;

		/*
		 * I don't want to muck with lost interrupts which come
		 * in while processing pending. By blocking interrupts, I
		 * can process all the pending ones, and then get signaled
		 * for any others when I unblock them. This should be just
		 * fine, since the number of times that irq_pending is true
		 * turns out to be near-zero.
		 */
		procmask(SIG_BLOCK);

		for (sig = 1; sig < NSIG; sig++) {
			if (NATIVEOS(sigismember)(&irq_sigpending, sig) > 0) {
				NATIVEOS(sigdelset)(&irq_sigpending, sig);
				process_signal(sig, 0, 0);
			}
		}
		irq_pending = 0;
		sig_blocked = 1;

		if (base_irq_nest == 0) {
			base_irq_nest = BASE_IRQ_SOFTINT_CLEARED |
				BASE_IRQ_SOFTINT_DISABLED;
			base_irq_softint_handler(NULL); /* See NOTE near prototype */
			base_irq_nest &= ~BASE_IRQ_SOFTINT_DISABLED;
		}
	}

	irq_enable = 1;
	if (sig_blocked) {
		sig_blocked = 0;
		procmask(SIG_UNBLOCK);
	}
}

void
osenv_intr_disable()
{
	irq_enable = 0;
}

int
osenv_intr_enabled()
{
	return irq_enable;
}

int
osenv_intr_save_disable(void)
{
	int enabled = irq_enable;
	irq_enable = 0;
	return enabled;
}

#define MAX_FDS	256
struct fd_callback {
	void 	(*cb)(void *);
	void	*arg;
} cb[MAX_FDS];

static fd_set	fd_in, fd_out;
static int	maxfds;

/*
 * call 'callback(arg)' when fd becomes readable
 */
void
oskitunix_register_async_fd(int fd, unsigned iotype,
			    void (*callback)(void *), void *arg)
{
	void		process_fds();
	static int	registered;

	if (fd >= MAX_FDS)
		panic("oskitunix_register_async_fd: fd %d too big", fd);

	cb[fd].cb = callback;
	cb[fd].arg = arg;

	if (iotype & IOTYPE_READ)
		FD_SET(fd, &fd_in);
	if (iotype & IOTYPE_WRITE)
		FD_SET(fd, &fd_out);

	/* Not perfect, but better than nothing. */
	if (fd > maxfds)
		maxfds = fd;

	if (!registered) {
		oskitunix_set_signal_handler(SIGIO, process_fds);
		registered = 1;
	}
}

void
oskitunix_unregister_async_fd(int fd)
{
	assert(fd < MAX_FDS);

	cb[fd].cb = 0;
	FD_CLR(fd, &fd_in);
	FD_CLR(fd, &fd_out);
}

/*
 * we received a SIGIO, figure which fd causes it and call appropriate callback
 */
static void
/* ARGSUSED */
process_fds(int signal, int code, struct sigcontext *scp)
{
	int	i, n;
	struct timeval poll;
	fd_set  in, out;

	in = fd_in;
	out = fd_out;
	memset(&poll, 0, sizeof poll);

	n = NATIVEOS(select)(maxfds + 1, &in, &out, 0, &poll);
	if (n > 0) {
		for (i = 0; i <= maxfds; i++) {
			if (FD_ISSET(i, &in) || FD_ISSET(i, &out)) {
				void (*callback)(void *) = cb[i].cb;
				cb[i].cb = 0;
				FD_CLR(i, &fd_in);
				FD_CLR(i, &fd_out);
				callback(cb[i].arg);
			}
		}
		return;
	}
	if (n < 0) {
		oskitunix_perror("select"), exit(0);

	}
	if (n == 0) {
	#if 0
		/* this actually happens a lot */
		printf(__FUNCTION__" received spurious SIGIO\n");
	#endif
	}
}
