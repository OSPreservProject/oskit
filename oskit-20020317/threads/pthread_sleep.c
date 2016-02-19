/*
 * Copyright (c) 1996, 1998, 1999, 2000 University of Utah and the Flux Group.
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

#include <threads/pthread_internal.h>

/*
 * Timed sleep.
 * 
 * The calling thread goes to sleep for the given amount of time, in
 * milliseconds. This is a primitive sleep, not implemented in terms
 * of pthread_cond_timedwait, since that takes an absolute time.
 */
int
oskit_pthread_sleep(oskit_s64_t timeout)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	int			rval, enabled;
	oskit_timespec_t	timespec = { 0, 0 };

	if (pthread == IDLETHREAD)
		panic("pthread_sleep: Idle thread tried to sleep");

	pthread_testcancel();

	assert_preemption_enabled();

	/*
	 * Must take the thread waitlock with interrupts blocked since
	 * the timeout is delivered at interrupt level.
	 */
	enabled = save_disable_interrupts();
	pthread_lock(&pthread->waitlock);

	/*
	 * Zero timeout means sleep until woken.
	 */
	if (timeout) {
		timespec.tv_sec  += timeout / 1000;
		timespec.tv_nsec += (timeout % 1000) * 1000000;

		rval = oskit_pthread_sleep_withflags(0, &timespec);
	}
	else
		rval = oskit_pthread_sleep_withflags(0, 0);

	restore_interrupt_enable(enabled);
	pthread_testcancel();

	return rval;
}

/*
 * Internal call to put a thread to sleep. Typically, there is an additional
 * waitflag associated with the sleep, so that the thread is waiting on
 * something to happen, but with a timeout to avoid infinite sleep. For example,
 * sigtimedwait() or the IPC functions that take a timeout value. To prevent
 * races, the timeout function must clear that additional flag. To make things
 * easier, the sleep specific flags are defined disjoint from all the other
 * flags, and the timeout code just clears the whole set. That should be okay
 * since the callers will know what is going on, and are going to look at the
 * return value, which will be ETIMEDOUT if the timer expired. 
 *
 * This must be called with interrupts disabled, and the pthread waitlock
 * held. The additional flag is added to the waitflags, and then removed
 * when the sleep terminates (via the timeout or an explicit wakeup).
 */
int
oskit_pthread_sleep_withflags(int waitflags, const oskit_timespec_t *timeout)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	int			rval;
	oskit_itimerspec_t	timespec;

	assert_interrupts_disabled();

	/*
	 * Zero timeout means sleep until woken.
	 */
	if (timeout) {
		timespec.it_value.tv_sec     = timeout->tv_sec;
		timespec.it_value.tv_nsec    = timeout->tv_nsec;
		timespec.it_interval.tv_sec  = 0;
		timespec.it_interval.tv_nsec = 0;
		oskit_timer_settime(pthread->sleeptimer, 0, &timespec);
		pthread->waitflags |= THREAD_WS_TIMERSET;
	}

	pthread->waitflags |=  waitflags;
	pthread->waitflags &= ~THREAD_WS_TIMEDOUT;
	pthread->waitflags |=  THREAD_WS_SLEEPING;
	
#ifdef  THREADS_DEBUG
	pthread_lock(&threads_sleepers_lock);
	threads_sleepers++;
	pthread_unlock(&threads_sleepers_lock);
#endif
	pthread_sched_reschedule(RESCHED_BLOCK, &pthread->waitlock);

#ifdef  THREADS_DEBUG
	pthread_lock(&threads_sleepers_lock);
	threads_sleepers--;
	pthread_unlock(&threads_sleepers_lock);
#endif
	pthread_lock(&pthread->waitlock);

	/*
	 * Check for timeout.
	 */
	if (pthread->waitflags & THREAD_WS_TIMEDOUT)
		rval = ETIMEDOUT;
	else
		rval = 0;

	pthread->waitflags &= ~(THREAD_WS_TIMERSET|THREAD_WS_TIMEDOUT);

	assert(pthread->waitflags == 0);

	pthread_unlock(&pthread->waitlock);
	return rval;
}

/*
 * Wake up a thread that put itself to sleep. This can get called out of
 * an interrupt because it is used in the select code. So, preemptions
 * and interrupts might be disabled. 
 */
int
oskit_pthread_wakeup(pthread_t tid)
{
	pthread_thread_t	*pthread;
	int			enabled;

	if ((pthread = tidtothread(tid)) == NULL_THREADPTR)
		return EINVAL;

	if (CURPTHREAD()->tid == tid)
		return EINVAL;

#ifdef THREADS_DEBUG_NOPE
	if (! IN_AN_INTERRUPT()) {
		assert_interrupts_enabled();
		assert_preemption_enabled();
	}
#endif
	/*
	 * Must block interrupts since the timeout is delivered at
	 * interrupt level.
	 */
	enabled = save_disable_interrupts();
	pthread_lock(&pthread->waitlock);

	if (pthread->waitflags & THREAD_WS_SLEEPING)
		pthread_wakeup_locked(pthread);
	else
		pthread_unlock(&pthread->waitlock);

	restore_interrupt_enable(enabled);
	return 0;
}

/*
 * Internal version. Thread lock is held. Interrupts are disabled.
 */
int
pthread_wakeup_locked(pthread_thread_t *pthread)
{
	assert_interrupts_disabled();
	
	/* disarm the timer. */
	if (pthread->waitflags & THREAD_WS_TIMERSET) {
		oskit_itimerspec_t	timespec = {{ 0, 0 }, { 0, 0 }};
		
		timespec.it_value.tv_sec  = 0;
		timespec.it_value.tv_nsec = 0;
		oskit_timer_settime(pthread->sleeptimer, 0, &timespec);
		pthread->waitflags &= ~THREAD_WS_TIMERSET;
	}

	/* Clear the secondary waitflags */
	pthread->waitflags &= THREAD_WS_SLEEPFLAGS;
	pthread->waitflags &= ~THREAD_WS_SLEEPING;
	pthread_unlock(&pthread->waitlock);
	
	return pthread_sched_setrunnable(pthread);
}

/*      
 * Timer expiration.
 */
oskit_error_t
pthread_sleep_timeout(struct oskit_iunknown *listener, void *arg)
{
	pthread_thread_t	*pthread = arg;
	int			enabled;

	/*
	 * Note that pthread waitlock is taken in an interrupt handler!
	 */

	/* Just in case some device driver renabled interrupts! */
	enabled = save_disable_interrupts();

	pthread_lock(&pthread->waitlock);

	DPRINTF("%p %d %p 0x%x\n",
		CURPTHREAD(), enabled, pthread, (int) pthread->waitflags);

	/*
	 * Already woken up?
	 */
	if (! (pthread->waitflags & THREAD_WS_SLEEPING)) {
		pthread_unlock(&pthread->waitlock);
		restore_interrupt_enable(enabled);
		return 0;
	}

	/*
	 * Nope, place it back on the runq. Request an AST if the current
	 * thread is no longer the thread that should be running.
	 *
	 * The secondary waitflags are also cleared.
	 */
	pthread->waitflags = THREAD_WS_TIMEDOUT;
	pthread_unlock(&pthread->waitlock);

	if (pthread_sched_setrunnable(pthread))
		softint_request(SOFTINT_ASYNCREQ);

	DPRINTF("ends ...\n");

	restore_interrupt_enable(enabled);
	return 0;
}
