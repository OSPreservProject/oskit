/*
 * Copyright (c) 1996, 1998, 1999, 2000, 2001 University of Utah and the Flux Group.
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
 * Posix signals, or something approximating them. The goal is to get
 * the obvious stuff nearly correct, but not go overboard.
 */

#include <threads/pthread_internal.h>
#include <strings.h>
#include <oskit/c/string.h>
#include <oskit/c/malloc.h>
#include "pthread_cond.h"
#include "pthread_signal.h"

/*
 * The sigactions array. Protected by its own lock.
 */
static struct sigaction sigactions[NSIG];
static pthread_lock_t	sigactions_lock = PTHREAD_LOCK_INITIALIZER;

/*
 * There is a global (or "process") set of pending signals.
 * kill() and sigqueue() affect the process pending set.
 */
static sigset_t		sigpending;

/*
 * A queue of all threads waiting in sigwait. 
 */
static queue_head_t	sigwaiters;

/*
 * An array of queues of pending signals posted with sigqueue(). 
 */
static queue_head_t	sigqueued[NSIG];

/*
 * We avoid malloc in interrupt handlers by preallocating the queue
 * entries for sigqueued above.
 */
static queue_head_t	sigqueue_free;

/*
 * Lock to protect the global signal stuff.
 */
static pthread_lock_t	siglock = PTHREAD_LOCK_INITIALIZER;

/*
 * Prototypes.
 */
int	pthread_kill_locked(pthread_thread_t *pthread, int signo);
void	really_deliver_signal(int sig, siginfo_t *code,struct sigcontext *scp);
void    oskit_deliver_async_signal(int sig);
void	oskit_deliver_process_signal(int sig);

int
pthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	int			err = 0;

	assert(pthread);

	/* siglock and pthread siglock are taken from an interrupt handler */
	assert_interrupts_enabled();
	disable_interrupts();
	pthread_lock(&pthread->siglock);

	if (oset)
		*oset = pthread->sigmask;

	if (set) {
		switch (how) {
		case SIG_BLOCK:
			pthread->sigmask |= *set;
			break;
		case SIG_UNBLOCK:
			pthread->sigmask &= ~*set;
			break;
		case SIG_SETMASK:
			pthread->sigmask = *set;
			break;
		default:
			err = EINVAL;
		}
	}

	/*
	 * Look for process pending signals that are unblocked, and deliver.
	 */
	pthread_lock(&siglock);
	while (sigpending & ~pthread->sigmask) {
		int sig = ffs(sigpending & ~pthread->sigmask);
	
		/* Call with siglock and thread siglock locked */
		oskit_deliver_process_signal(sig);
	}
	pthread_unlock(&siglock);

	/*
	 * Look for pthread pending signals that are unblocked, and deliver.
	 */
	while (pthread->sigpending & ~pthread->sigmask) {
		int sig = ffs(pthread->sigpending & ~pthread->sigmask);

		/* Call with thread siglock locked */
		oskit_deliver_async_signal(sig);
	}

	pthread_unlock(&pthread->siglock);
	enable_interrupts();
	return err;
}

/*
 * This can be called out of an interrupt handler, say from an alarm
 * expiration. Therefore, must not take the pthread lock. We use a
 * separate lock to protect the signal state.
 */
int
pthread_kill(pthread_t tid, int signo)
{
	pthread_thread_t	*pthread;
	int			enabled;

	/* Error check? Sure! */
	if (!signo)
		return 0;

	if (signo < 0 || signo >= NSIG)
		return EINVAL;

	if ((pthread = tidtothread(tid)) == NULL_THREADPTR)
		return EINVAL;

	/* siglock and pthread siglock are taken from an interrupt handler */
	enabled = save_disable_interrupts();
	pthread_lock(&pthread->siglock);

	/*
	 * pthread_kill_locked() will take care of unlocking.
	 */
	pthread_kill_locked(pthread, signo);

	/*
	 * If not in an interrupt, use this opportunity to deliver
	 * pending unblocked signals to the current thread.
	 */
	if (! IN_AN_INTERRUPT()) {
		pthread = CURPTHREAD();
		SIGCHECK(pthread);
	}

	restore_interrupt_enable(enabled);
	return 0;
}

/*
 * This does all the work.
 *
 * Should be called with interrupts disabled, and siglock and pthread
 * siglock are held.
 * Returns with the locks released (caller must restore interrupts).
 */
int
pthread_kill_locked(pthread_thread_t *pthread, int signo)
{
	/*
	 * Look at the process sigactions. If the "process" is ignoring
	 * the signal, then the signal is not placed in the pending list.
	 */
	pthread_lock(&sigactions_lock);
	if (sigactions[signo].sa_handler == SIG_IGN) {
		pthread_unlock(&pthread->siglock);
		pthread_unlock(&sigactions_lock);
		return 0;
	}
	/* Don't need the global siglock anymore. */
	pthread_unlock(&sigactions_lock);

	/*
	 * First have to check for sigwaiting, since that overrides sigmask.
	 */
	pthread_lock(&pthread->waitlock);
	if ((pthread->waitflags & THREAD_WS_SIGWAIT) &&
	    sigismember(&pthread->sigwaiting, signo)) {
		sigaddset(&pthread->sigpending, signo);
		sigemptyset(&pthread->sigwaiting);
		pthread_unlock(&pthread->siglock);
		/* pthread waitlock will be released. */
		pthread_wakeup_locked(pthread);
		return 0;
	}
	pthread_unlock(&pthread->waitlock);

	/*
	 * Add the signal to list of pending signals for the target thread.
	 */
	sigaddset(&pthread->sigpending, signo);

	/*
	 * If the signal is currently blocked, then do nothing else.
	 * It will be noticed when the signal is unblocked.
	 */
	if (sigismember(&pthread->sigmask, signo)) {
		pthread_unlock(&pthread->siglock);
		return 0;
	}

	/*
	 * Is the current thread sending itself a signal? This is okay, as
	 * long as its not from within an interrupt handler, say, from an
	 * alarm expiration. The caller is going to look for pending signals
	 * to the current thread. 
	 *
	 * If this *is* from within an interrupt, must wait until later
	 * since delivering it now is really the wrong way to go. Lets
	 * schedule a soft interrupt, and get the thread switched out.
	 * The signal will be delivered when it comes back in.
	 */
	if (pthread == CURPTHREAD()) {
		if (IN_AN_INTERRUPT()) {
			softint_request(SOFTINT_ASYNCREQ);
		}
		pthread_unlock(&pthread->siglock);
		return 0;
	}

	/* Don't need the pthread siglock anymore; waking up the thread */
	pthread_unlock(&pthread->siglock);

	/*
	 * Below here, we muck with the waitflags, which are accessed
	 * from interrupt handlers. Interrupts are already disabled,
	 * so take the thread waitlock.
	 */
	pthread_lock(&pthread->waitlock);

	/*
	 * If the thread is in an osenv_sleep(), issue a wakeup.
	 * The thread will be allowed to return through the sleep, to
	 * be caught sometime later. This allows driver state to be
	 * cleaned up before the thread is actually signaled.
	 */
	if (pthread->waitflags & THREAD_WS_OSENVSLEEP) {
		pthread_unlock(&pthread->waitlock);
		osenv_wakeup(pthread->sleeprec, OSENV_SLEEP_CANCELED);
		return 0;
	}

	/*
	 * If the thread is THREAD_WS_SLEEPING, then restart it. 
	 */
	if (pthread->waitflags & THREAD_WS_SLEEPING) {
		/* pthread waitlock will be released. */
		pthread_wakeup_locked(pthread);
		return 0;
	}

	/*
	 * If the thread is THREAD_WS_CONDWAIT, then restart it. The wrinkle
	 * is a race condition between the time the thread is taken off
	 * the condition queue and the time the thread state is changed.
	 */
	if (pthread->waitflags & THREAD_WS_CONDWAIT) {
		struct pthread_cond_impl *pimpl = pthread->waitcond->impl;

		pthread_lock(&(pimpl->lock));

		/*
		 * The thread was still on the Q, so its safe to change
		 * its state to reflect that it is not longer waiting
		 * on the condition.
		 *
		 * If the thread was not on the Q, we caught the race,
		 * and do not have to do anything.
		 */
		if (pthread_remove_fromQ(&(pimpl->waiters), pthread)) {
			pthread->waitflags &= ~THREAD_WS_CONDWAIT;
			pthread->waitcond   = 0;
			pthread_unlock(&(pimpl->lock));
			pthread_unlock(&pthread->waitlock);
			pthread_sched_setrunnable(pthread);
			return 0;
		}

		pthread_unlock(&(pimpl->lock));
		pthread_unlock(&pthread->waitlock);
		return 0;
	}

	/*
	 * IPC Wait.
	 */
	if (pthread->waitflags & THREAD_WS_IPCWAIT_FLAG) {
		/* pthread waitlock will be released. */
		pthread_ipc_cancel(pthread);
		return 0;
	}

	/*
	 * Done with waitflags.
	 */
	pthread_unlock(&pthread->waitlock);

	/*
	 * Must be running on another CPU. Must wait for it to be noticed.
	 */
	return 0;
}

/*
 * The rest of the Posix signal code. This stuff replaces the corresponding
 * code in the Posix library, since I don't see how it can be done unless
 * its all in one place.
 */

/*
 * sigaction
 */
int
sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	if (sig < 0 || sig >= NSIG)
		return errno = EINVAL, -1;

	assert_preemption_enabled();
	
	/* siglock and pthread siglock are taken from an interrupt handler */
	assert_interrupts_enabled();
	disable_interrupts();
	pthread_lock(&sigactions_lock);

	if (oact)
		*oact = sigactions[sig];
	if (act)
		sigactions[sig] = *act;

	pthread_unlock(&sigactions_lock);

	/*
	 * If the action for this signal is being set to SIG_IGN or SIG_DFL,
	 * and that signal is process pending, then clear it.
	 */
	pthread_lock(&siglock);
	if (act && (act->sa_handler == SIG_IGN ||
		    act->sa_handler == SIG_DFL)) {
		while (! queue_empty(&sigqueued[sig])) {
			sigqueue_thingie_t	*pthingie;

			queue_remove_first(&sigqueued[sig], pthingie,
					   sigqueue_thingie_t *, chain);
			queue_enter(&sigqueue_free, pthingie,
				    sigqueue_thingie_t *, chain);
		}
		sigdelset(&sigpending, sig);
	}
	pthread_unlock(&siglock);
	
	enable_interrupts();
	return 0;
}

/*
 * sigprocmask. In a multithreaded program, this is just pthread_sigmask
 */
int
sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
	return pthread_sigmask(how, set, oset);
}

/*
 * raise. In a multithreaded program, raise is pthread_kill on itself.
 */
int
raise(int sig)
{
	return pthread_kill(pthread_self(), sig);
}

/*
 * kill. What does it mean to kill() in a multithreaded program? The POSIX
 * spec says that a signal sent to a "process" shall be delivered to only
 * one thread. If no thread has that signal unblocked, then the first
 * thread to unblock the signal is the lucky winner. Well, that means we
 * need to have a global sigpending to record process pending signals.
 */
int
kill(pid_t pid, int signo)
{
	pthread_thread_t	*pthread;
	int			enabled, i;
	struct sigaction	act;
	extern pthread_lock_t	pthread_create_lock;

	/* Error check? Sure! */
	if (!signo)
		return 0;

	/* siglock and pthread siglock are taken from an interrupt handler */
	enabled = save_disable_interrupts();

	/*
	 * Look at the process sigactions. If the "process" is ignoring
	 * the signal, then the signal is not placed in the pending list.
	 */
	pthread_lock(&sigactions_lock);
	act = sigactions[signo];
	pthread_unlock(&sigactions_lock);
	
	if (act.sa_handler == SIG_IGN) {
		restore_interrupt_enable(enabled);
		return 0;
	}

	/*
	 * Take and hold the global siglock. This is needed to prevent
	 * a race with sigwait.
	 */
	pthread_lock(&siglock);

	/*
	 * Kill does not queue. If the signal is already pending, this
	 * one is tossed.
	 */
	if (sigismember(&sigpending, signo)) {
		pthread_unlock(&siglock);
		restore_interrupt_enable(enabled);
		return 0;
	}

	/*
	 * Make the signal process pending.
	 */
	sigaddset(&sigpending, signo);

	/*
	 * Look through the threads in sigwait to see if any of them
	 * is waiting for the signal. This is done as a separate pass
	 * since the value of the pthread sigmask is ignored (threads
	 * in sigwait will have blocked the signals being waited for).
	 */
	queue_iterate(&sigwaiters, pthread, pthread_thread_t *, chain) {
		assert(pthread);
		pthread_lock(&pthread->siglock);
		pthread_lock(&pthread->waitlock);
		if ((pthread->waitflags & THREAD_WS_SIGWAIT) &&
		    sigismember(&pthread->sigwaiting, signo)) {
			/*
			 * Bingo. Make the signal thread pending and
			 * wake it up.
			 */
			sigaddset(&pthread->sigpending, signo);
			sigemptyset(&pthread->sigwaiting);
			pthread_unlock(&pthread->siglock);
			/* pthread waitlock will be released. */
			pthread_wakeup_locked(pthread);
			pthread_unlock(&siglock);
			restore_interrupt_enable(enabled);
			return 0;
		}
		pthread_unlock(&pthread->waitlock);
		pthread_unlock(&pthread->siglock);
	}

	/*
	 * No threads in sigwait. Too bad. Must find another thread to
	 * deliver it to.
	 */
	pthread_lock(&pthread_create_lock);
	for (i = 1; i < THREADS_MAX_THREAD; i++) {
		if ((pthread = threads_tidtothread[i]) != NULL_THREADPTR) {
			pthread_lock(&pthread->siglock);
			if (! sigismember(&pthread->sigmask, signo)) {
				pthread_unlock(&pthread_create_lock);
				
				/* Thread siglock will be released. */
				pthread_kill_locked(pthread, signo);
				break;
			}
			pthread_unlock(&pthread->siglock);
		}
	}
	pthread_unlock(&pthread_create_lock);
	pthread_unlock(&siglock);

	/*
	 * If not in an interrupt, use this opportunity to deliver
	 * pending unblocked signals to the current thread.
	 */
	if (! IN_AN_INTERRUPT()) {
		pthread = CURPTHREAD();
		SIGCHECK(pthread);
	}

	restore_interrupt_enable(enabled);
	return 0;
}

/*
 * sigqueue.
 */
int
sigqueue(pid_t pid, int signo, const union sigval value)
{
	return sigqueue_internal(pid, signo, value, SI_QUEUE);
}

/*
 * sigqueue internal
 */
int
sigqueue_internal(pid_t pid, int signo, const union sigval value, int code)
{
	pthread_thread_t	*pthread;
	int			enabled;
	int			i;
	sigqueue_thingie_t	*pthingie = NULL;
	struct sigaction	act;
	extern pthread_lock_t	pthread_create_lock;

	/* Error check? Sure! */
	if (!signo)
		return 0;

	/* siglock and pthread siglock are taken from an interrupt handler */
	enabled = save_disable_interrupts();

	/*
	 * Look at the process sigactions. If the "process" is ignoring
	 * the signal, then the signal is not placed in the pending list.
	 */
	pthread_lock(&sigactions_lock);
	act = sigactions[signo];
	pthread_unlock(&sigactions_lock);
	
	if (act.sa_handler == SIG_IGN) {
		restore_interrupt_enable(enabled);
		return 0;
	}

	/*
	 * Take and hold the global siglock. This is needed to prevent
	 * a race with sigwait.
	 */
	pthread_lock(&siglock);

	/*
	 * If the flags does not include SA_SIGINFO, and there is already
	 * a signal pending, this new one is dropped.
	 */
	if ((! (act.sa_flags & SA_SIGINFO)) &&
	    sigismember(&sigpending, signo)) {
		pthread_unlock(&siglock);
		restore_interrupt_enable(enabled);
		return 0;
	}

	/*
	 * Gotta have space for the new signal.
	 */
	if (queue_empty(&sigqueue_free)) {
		pthread_unlock(&siglock);
		restore_interrupt_enable(enabled);
		return EAGAIN;
	}

	/*
	 * Create a queue entry.
	 */
	queue_remove_first(&sigqueue_free,
			   pthingie, sigqueue_thingie_t *, chain);

	pthingie->info.si_signo = signo;
	pthingie->info.si_code  = code;
	pthingie->info.si_value = value;

	/*
	 * Queue the signal on the process.
	 */
	queue_enter(&sigqueued[signo], pthingie, sigqueue_thingie_t *, chain);
	sigaddset(&sigpending, signo);

	/*
	 * Look through the threads in sigwait to see if any of them
	 * is waiting for the signal. This is done as a separate pass
	 * since the value of the pthread sigmask is ignored (threads
	 * in sigwait will have blocked the signals being waited for).
	 * If we find one, wakeup that thread. Note that POSIX says that
	 * if multiple threads are sigwaiting for the same signal number,
	 * exactly one thread is woken up. The problem is how to maintain
	 * the FIFO order, and how to prevent lost signals in the case that
	 * a thread calls sigwait before the woken thread runs and gets it.
	 */
	queue_iterate(&sigwaiters, pthread, pthread_thread_t *, chain) {
		pthread_lock(&pthread->siglock);
		pthread_lock(&pthread->waitlock);
		if ((pthread->waitflags & THREAD_WS_SIGWAIT) &&
		    sigismember(&pthread->sigwaiting, signo)) {
			/*
			 * Bingo. Wake it up.
			 */
			sigemptyset(&pthread->sigwaiting);
			pthread_unlock(&pthread->siglock);
			/* pthread waitlock will be released. */
			pthread_wakeup_locked(pthread);
			pthread_unlock(&siglock);
			restore_interrupt_enable(enabled);
			return 0;
		}
		pthread_unlock(&pthread->waitlock);
		pthread_unlock(&pthread->siglock);
	}

	/*
	 * Need to find a thread to deliver the signal to. Look for the
	 * first thread that is not blocking the signal, and send it the
	 * signal. It is my opinion that any program that is using sigwait,
	 * and has not blocked signals in all of its threads, is bogus. The
	 * same is true if the program is not using sigwait, and has the
	 * signal unblocked in more than one thread.
	 * Why? You might wake up a thread, but not have an actual queue
	 * entry left by the time it runs again and looks, since another
	 * thread could call sigwait and get that queue entry, or if there
	 * are multiple threads that can take the signal, one thread could
	 * get all the entries. This could result in an interrupted thread,
	 * but with no signal to deliver. Well, not much to do about it.
	 * Lets just queue the signal for the process, and let the chips
	 * fall where they may. 
	 */
	pthread_lock(&pthread_create_lock);
	for (i = 1; i < THREADS_MAX_THREAD; i++) {
		if ((pthread = threads_tidtothread[i]) != NULL_THREADPTR) {
			pthread_lock(&pthread->siglock);
			if (! sigismember(&pthread->sigmask, signo)) {
				pthread_unlock(&pthread_create_lock);
				
				/* Thread siglock will be released. */
				pthread_kill_locked(pthread, signo);
				break;
			}
			pthread_unlock(&pthread->siglock);
		}
	}
	pthread_unlock(&pthread_create_lock);
	pthread_unlock(&siglock);

	/*
	 * If not in an interrupt, use this opportunity to deliver
	 * pending unblocked signals to the current thread.
	 */
	if (! IN_AN_INTERRUPT()) {
		pthread = CURPTHREAD();
		SIGCHECK(pthread);
	}

	restore_interrupt_enable(enabled);
	return 0;
}

/*
 * Sigwait. Sigwait overrides the state of the pthread sigmask and the global
 * sigactions. The caller *must* block the set of signals in "set", before
 * calling sigwait, otherwise the behaviour is undefined (which means that
 * the caller will take an async signal anyway, and sigwait will return EINTR.
 */
static oskit_error_t
oskit_sigwait_internal(const sigset_t *set,
		       siginfo_t *info, const oskit_timespec_t *timeout)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	int			thissig, rval;

	pthread_testcancel();

	assert_preemption_enabled();

	/* siglock and pthread siglock are taken from an interrupt handler */
	assert_interrupts_enabled();
	disable_interrupts();

	/*
	 * First check for process pending signals. Must take and hold
	 * the global siglock to prevent races with kill() and sigqueue().
	 */
	pthread_lock(&pthread->siglock);
	pthread_lock(&siglock);
	if (sigpending & *set) {
		sigqueue_thingie_t     *pthingie;
		
		thissig = ffs(sigpending & *set);

		/*
		 * Sent with kill(). Using sigwait and kill is Bogus!
		 */
		if (queue_empty(&sigqueued[thissig])) {
			info->si_signo           = thissig;
			info->si_code            = SI_USER;
			info->si_value.sival_int = 0;

			sigdelset(&pthread->sigpending, thissig);
			sigdelset(&sigpending, thissig);
			pthread_unlock(&siglock);
			pthread_unlock(&pthread->siglock);
			enable_interrupts();
			return 0;
		}

		/*
		 * Grab the first queue entry.
		 */
		queue_remove_first(&sigqueued[thissig],
				   pthingie, sigqueue_thingie_t *, chain);

		/*
		 * If that was the last one, reset the process sigpending.
		 */
		if (queue_empty(&sigqueued[thissig]))
			sigdelset(&sigpending, thissig);
		sigdelset(&pthread->sigpending, thissig);

		/*
		 * Copy the information and free the queue entry.
		 */
		info->si_signo           = pthingie->info.si_signo;
		info->si_code            = pthingie->info.si_code;
		info->si_value.sival_int = pthingie->info.si_value.sival_int;
		queue_enter(&sigqueue_free,
			    pthingie, sigqueue_thingie_t *, chain);

		pthread_unlock(&siglock);
		pthread_unlock(&pthread->siglock);
		enable_interrupts();
		return 0;
	}
		
	/*
	 * Now check for pthread pending signals.
	 */
	if (pthread->sigpending & *set) {
		thissig = ffs(pthread->sigpending & *set);
		info->si_signo           = thissig;
		info->si_code            = SI_USER;
		info->si_value.sival_int = 0;
		sigdelset(&pthread->sigpending, thissig);
		pthread_unlock(&siglock);
		pthread_unlock(&pthread->siglock);
		enable_interrupts();
		return 0;
	}

	/*
	 * For timed wait, if nothing is available and the timeout value
	 * is zero, its an error.
	 */
	if (timeout && timeout->tv_sec == 0 && timeout->tv_nsec == 0) {
		pthread_unlock(&siglock);
		pthread_unlock(&pthread->siglock);
		enable_interrupts();
		return EAGAIN;
	}

	/*
	 * Grab the wait lock and set the sigwaiting mask. Once that is done,
	 * release the thread siglock; Another thread can try and wake this
	 * thread up as a result of seeing it in sigwait, but the actual
	 * wakeup will be delayed until the waitlock is released in the switch
	 * code.
	 */
	pthread_lock(&pthread->waitlock);
	pthread->sigwaiting = *set;
	pthread_unlock(&pthread->siglock);

	/*
	 * Add this thread to the list of threads in sigwait. Once that is
	 * done, it is safe to release the global siglock, which will allow
	 * another thread to scan the sigwaiters list. As above, it might
	 * find a thread in sigwait, but it will not be able to wake it up
	 * until the waitlock is released in the switch code.
	 */
	queue_enter(&sigwaiters, pthread, pthread_thread_t *, chain);
	pthread_unlock(&siglock);

	/* and block */
	rval = oskit_pthread_sleep_withflags(THREAD_WS_SIGWAIT, timeout);

	pthread_lock(&pthread->siglock);
	pthread->sigwaiting = 0;

	/*
	 * Remove from the list of threads in sigwait.
	 */
	pthread_lock(&siglock);
	queue_remove(&sigwaiters, pthread, pthread_thread_t *, chain);

	/*
	 * Check for cancelation.
	 *
	 * XXX hacky, need to drop all our locks in case the cancelation
	 * happens.
	 */
	pthread_unlock(&siglock);
	pthread_unlock(&pthread->siglock);
	pthread_testcancel();

	/*
	 * While the locks are down, check for the timed-out case.
	 */
	if (rval == ETIMEDOUT) {
		enable_interrupts();
		return EAGAIN;
	}

	pthread_lock(&pthread->siglock);
	pthread_lock(&siglock);

	/*
	 * Look for a wakeup to deliver a queued signal. This would come
	 * either from kill() or from sigqueue().
	 */
	if (sigpending & *set) {
		sigqueue_thingie_t     *pthingie;
		
		thissig = ffs(sigpending & *set);

		/*
		 * Sent with kill(). Using sigwait and kill is Bogus!
		 */
		if (queue_empty(&sigqueued[thissig])) {
			info->si_signo           = thissig;
			info->si_code            = SI_USER;
			info->si_value.sival_int = 0;

			sigdelset(&sigpending, thissig);
			pthread_unlock(&siglock);
			pthread_unlock(&pthread->siglock);
			enable_interrupts();
			return 0;
		}

		/*
		 * Grab the first queue entry.
		 */
		queue_remove_first(&sigqueued[thissig],
				   pthingie, sigqueue_thingie_t *, chain);

		/*
		 * If that was the last one, reset the process sigpending.
		 */
		if (queue_empty(&sigqueued[thissig]))
			sigdelset(&sigpending, thissig);

		/*
		 * Copy the information and free the queue entry.
		 */
		info->si_signo           = pthingie->info.si_signo;
		info->si_code            = pthingie->info.si_code;
		info->si_value.sival_int = pthingie->info.si_value.sival_int;
		queue_enter(&sigqueue_free,
			    pthingie, sigqueue_thingie_t *, chain);

		pthread_unlock(&siglock);
		pthread_unlock(&pthread->siglock);
		enable_interrupts();
		return 0;
	}
	
	/*
	 * Well, at the moment I am going to assume that if this thread
	 * wakes up, and there is no signal pending in the waitset, the
	 * thread wait was interrupted for some other reason. Return EINTR.
	 */
	if (! (pthread->sigpending & *set)) {
		pthread_unlock(&siglock);
		pthread_unlock(&pthread->siglock);
		enable_interrupts();
		return EINTR;
	}

	/*
	 * Otherwise, get the first signal and return it.
	 */
	thissig = ffs(pthread->sigpending & *set);
	info->si_signo           = thissig;
	info->si_code            = SI_USER;
	info->si_value.sival_int = 0;
	sigdelset(&pthread->sigpending, thissig);
	pthread_unlock(&siglock);
	pthread_unlock(&pthread->siglock);
	
	enable_interrupts();
	return 0;
}

/*
 * Sigwait.
 */
int
sigwait(const sigset_t *set, int *sig)
{
	siginfo_t	info;
	oskit_error_t	rc;

	memset(&info, 0, sizeof(info));

	rc = oskit_sigwait_internal(set, &info, 0);
	if (rc)
		return rc;

	*sig = info.si_signo;
	return 0;
}

/*
 * Sigwaitinfo. 
 */
int
sigwaitinfo(const sigset_t *set, siginfo_t *info)
{
	oskit_error_t	rc;

	rc = oskit_sigwait_internal(set, info, 0);
	if (rc)
		return rc;

	return 0;
}

/*
 * Sigtimedwait.
 */
int
sigtimedwait(const sigset_t *set,
	     siginfo_t *info, const struct oskit_timespec *timeout)
{
	oskit_error_t	rc;

	if (! timeout)
		return EINVAL;
	
	rc = oskit_sigwait_internal(set, info, timeout);
	if (rc)
		return rc;

	return 0;
}

/*
 * XXX we could probably implement this
 */
int
sigsuspend(const sigset_t *sigmask)
{
	return ENOSYS;
}

/*
 * Internal stuff.
 */

/*
 * Deliver a signal generated from a trap. This is the upcall from the
 * machine dependent code that created the sigcontext structure from
 * the trap state.
 */
void
oskit_libc_sendsig(int sig, int code, struct sigcontext *scp)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	siginfo_t		siginfo;
	int			enabled;

	siginfo.si_signo           = sig;
	siginfo.si_code            = SI_EXCEP;
	siginfo.si_value.sival_int = code;

	/*
	 * Need to call really_deliver_signal() with interrupts blocked
	 * and the pthread siglock held.
	 */
	enabled = save_disable_interrupts();
	pthread_lock(&pthread->siglock);

	really_deliver_signal(sig, &siginfo, scp);

	pthread_unlock(&pthread->siglock);
	restore_interrupt_enable(enabled);
}

/*
 * Deliver an asynchronous signal. This must be called with interrupts
 * blocked and the pthread siglock held. 
 */
void
oskit_deliver_async_signal(int sig)
{
	struct sigcontext	sc;
	siginfo_t		siginfo;

	/* create a stub sigcontext_t. */
	bzero(&sc, sizeof(sc));

	siginfo.si_signo           = sig;
	siginfo.si_code            = SI_USER;
	siginfo.si_value.sival_int = 0;

	really_deliver_signal(sig, &siginfo, &sc);
}

/*
 * Deliver a process signals. This must be called with interrupts
 * blocked and the siglock and pthread siglock held.
 */
void
oskit_deliver_process_signal(int sig)
{
	struct sigcontext	sc;
	siginfo_t		siginfo;
	sigqueue_thingie_t     *pthingie;

	/* create a stub sigcontext_t. */
	bzero(&sc, sizeof(sc));

	/*
	 * Sent with kill(). Using sigwait and kill is Bogus!
	 */
	if (queue_empty(&sigqueued[sig])) {
		siginfo.si_signo           = sig;
		siginfo.si_code            = SI_USER;
		siginfo.si_value.sival_int = 0;

		sigdelset(&sigpending, sig);
		goto deliver;
	}

	/*
	 * Grab the first queue entry.
	 */
	queue_remove_first(&sigqueued[sig],
			   pthingie, sigqueue_thingie_t *, chain);

	/*
	 * If that was the last one, reset the process sigpending.
	 */
	if (queue_empty(&sigqueued[sig]))
		sigdelset(&sigpending, sig);

	/*
	 * Copy the information and free the queue entry.
	 */
	siginfo.si_signo           = pthingie->info.si_signo;
	siginfo.si_code            = pthingie->info.si_code;
	siginfo.si_value.sival_int = pthingie->info.si_value.sival_int;
	queue_enter(&sigqueue_free, pthingie, sigqueue_thingie_t *, chain);

	/*
	 * Release the global siglock for the delivery.
	 */
 deliver:
	pthread_unlock(&siglock);

	really_deliver_signal(sig, &siginfo, &sc);

	/*
	 * Reacquire since the caller expects it.
	 */
	pthread_lock(&siglock);
}

/*
 * Deliver any pending signals. Called out of the context switch code
 * when a thread switches in, and there are pending signals.
 *
 * Interrupts are blocked and the thread siglock is locked.
 */
void
oskit_deliver_pending_signals(void)
{
	pthread_thread_t	*pthread = CURPTHREAD();

	/*
	 * Look for process pending signals that are unblocked, and deliver.
	 */
	pthread_lock(&siglock);
	while (sigpending & ~pthread->sigmask) {
		int sig = ffs(sigpending & ~pthread->sigmask);
	
		/* Call with siglock and thread siglock locked */
		oskit_deliver_process_signal(sig);
	}
	pthread_unlock(&siglock);

	/*
	 * Now deliver any pthread pending signals that are left.
	 */
	while (pthread->sigpending & ~pthread->sigmask) {
		int sig = ffs(pthread->sigpending & ~pthread->sigmask);

		/* Call at splhigh and thread locked */
		oskit_deliver_async_signal(sig);
	}
}

/*
 * Actually deliver the signal to the thread. At this point the signal
 * is going to be delivered, so it no longer matters if it is blocked.
 */
void
really_deliver_signal(int sig, siginfo_t *info, struct sigcontext *scp)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	sigset_t		sigmask, oldmask;
	struct sigaction	act;
	int			enabled;

	assert_interrupts_disabled();
	save_preemption_enable(enabled);

	pthread_lock(&sigactions_lock);
	act = sigactions[sig];
	pthread_unlock(&sigactions_lock);

	/*
	 * Ignored?
	 */
	if (act.sa_handler == SIG_IGN || act.sa_handler == SIG_ERR)
		return;

	if (act.sa_handler == SIG_DFL) {
		/* Default action for all signals is termination */
		if (info->si_code == SI_EXCEP) {
			printf("TID %p: exception, code=0x%x\n",
			       pthread->tid, info->si_value.sival_int);
			sigcontext_dump(scp);
		}
		panic("Sendsig: Signal %d caught but no handler", sig);
	}

	/*
	 * Set the signal mask for calling the handler.
	 */
	oldmask = sigmask = pthread->sigmask;
	sigaddset(&sigmask, sig);
	sigmask |= act.sa_mask;
	sigdelset(&pthread->sigpending, sig);
	pthread->sigmask = sigmask;
	pthread_unlock(&pthread->siglock);
	enable_interrupts();
	enable_preemption();

	/*
	 * and call the handler ...
	 */
	if (act.sa_flags & SA_SIGINFO)
		act.sa_sigaction(sig, info, (void *) scp);
	else
		((void (*)(int, int, struct sigcontext *))act.sa_handler)
			(sig, info->si_value.sival_int, scp);

	disable_interrupts();
	restore_preemption_enable(enabled);
	pthread_lock(&pthread->siglock);
	pthread->sigmask = oldmask;

	/*
	 * and return with thread siglock locked at splhigh.
	 */
}

void
pthread_init_signals()
{
	int			i;
	sigqueue_thingie_t	*pthingie;
	extern void libc_sendsig_init(void);

	queue_init(&sigwaiters);
	queue_init(&sigqueue_free);
	
	/* Initialize the default signal actions. */
	for (i = 0; i < NSIG; i++)
		sigactions[i].sa_handler = SIG_DFL;

	/* Initialize the signal queue headers. */
	for (i = 0; i < NSIG; i++)
		queue_init(&sigqueued[i]);

	/* Create a free list of queue thingies. */
	if ((pthingie = (sigqueue_thingie_t *)
	                smalloc(sizeof(sigqueue_thingie_t) * SIGQUEUE_MAX))
	    == NULL)
		panic("pthread_init_signals: Out of memory");

	for (i = 0; i < SIGQUEUE_MAX; i++) {
		queue_enter(&sigqueue_free,
			    pthingie, sigqueue_thingie_t *, chain);
		pthingie++;
	}

	libc_sendsig_init();
}

void
signals_init(void)
{
	/* Called by libc. Initialization was already done above. */
}
