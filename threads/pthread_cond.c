/*
 * Copyright (c) 1998, 1999, 2000 University of Utah and the Flux Group.
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

#include <malloc.h>
#include <threads/pthread_internal.h>
#include "pthread_cond.h"

/*
 * Create and initialize a new condition variable
 */
int
pthread_cond_init(pthread_cond_t *c, const pthread_condattr_t *attr)
{
	struct pthread_cond_impl	*pimpl;
	
	if (c == NULL)
		return EINVAL;
	
	if (! attr)
		attr = &pthread_condattr_default;

	if ((pimpl =
	     (struct pthread_cond_impl *) smalloc(sizeof(*pimpl))) == NULL)
		return ENOMEM;

        queue_init(&(pimpl->waiters));
	pthread_lock_init(&(pimpl->lock));
	c->impl = pimpl;

        return 0;
}

/*
 * Destroy a condition variable by deallocating the implementation.
 */
int
pthread_cond_destroy(pthread_cond_t *c)
{
	struct pthread_cond_impl	*pimpl;
	int				enabled;

	if (c == NULL || ((pimpl = c->impl) == NULL))
		return EINVAL;

	save_preemption_enable(enabled);
	disable_preemption();

	pthread_lock(&(pimpl->lock));

	if (! queue_empty(&(pimpl->waiters))) {
                printf("pthread_cond_destroy: "
		       "condition variable %p/%p still in use\n", c, pimpl);
		pthread_unlock(&(pimpl->lock));
		restore_preemption_enable(enabled);
		return EINVAL;
        }
	
	sfree(pimpl, sizeof(*pimpl));
	c->impl = NULL;
	restore_preemption_enable(enabled);
        return 0;
}

/*
 * Wait on a condition. The timedwait version is below. Because of the
 * timer, conditions have to be manipulated at splhigh to avoid deadlock
 * with the timer interrupt, which needs to lock both the thread and the
 * condition so it can remove the thread from the waitlist and restart
 * it. The length of time at splhigh is unfortunate.
 */
int
pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{
	pthread_thread_t		*pthread = CURPTHREAD();
	struct pthread_cond_impl	*pimpl;
	int				enabled, rc;

	pthread_testcancel();

	if (c->impl == NULL && ((rc = pthread_cond_init(c, NULL))))
		return rc;

	save_preemption_enable(enabled);
	disable_preemption();

	pimpl = c->impl;

	/* Must block interrupts before taking condvar lock or waitlock */
	assert_interrupts_enabled();
	disable_interrupts();
	
	pthread_lock(&(pimpl->lock));

        /* place ourself on the queue */
	queue_check(&(pimpl->waiters), pthread);
	queue_enter(&(pimpl->waiters), pthread, pthread_thread_t *, chain);
	
        /* unlock mutex */
        pthread_mutex_unlock(m);

	pthread_lock(&pthread->waitlock);
	pthread_unlock(&(pimpl->lock));

	pthread->waitflags |= THREAD_WS_CONDWAIT;
	pthread->waitcond   = c;

	/* and block */
	pthread_sched_reschedule(RESCHED_BLOCK, &pthread->waitlock);

	assert_interrupts_disabled();
	enable_interrupts();
	restore_preemption_enable(enabled);

        /* grab mutex before returning */
        pthread_mutex_lock(m);

	/* Look for a cancelation point */
	pthread_testcancel();

        return 0;
}

/*
 * Internal (safe) version that does not call pthread_testcancel.
 */
int
pthread_cond_wait_safe(pthread_cond_t *c, pthread_mutex_t *m)
{
	pthread_thread_t		*pthread = CURPTHREAD();
	struct pthread_cond_impl	*pimpl;
	int				enabled, preemptable;

	assert(c && c->impl);

	save_preemption_enable(preemptable);
	disable_preemption();

	pimpl = c->impl;

	/* Must block interrupts before taking condvar lock or waitlock */
	enabled = save_disable_interrupts();

	pthread_lock(&(pimpl->lock));

        /* place ourself on the queue */
	queue_check(&(pimpl->waiters), pthread);
	queue_enter(&(pimpl->waiters), pthread, pthread_thread_t *, chain);
	
        /* unlock mutex */
        pthread_mutex_unlock(m);

	pthread_lock(&pthread->waitlock);
	pthread_unlock(&(pimpl->lock));

	pthread->waitflags |= THREAD_WS_CONDWAIT;
	pthread->waitcond   = c;

#ifdef  THREADS_DEBUG
	pthread_lock(&threads_sleepers_lock);
	threads_sleepers++;
	pthread_unlock(&threads_sleepers_lock);
#endif
	/* and block */
	pthread_sched_reschedule(RESCHED_BLOCK, &pthread->waitlock);

#ifdef  THREADS_DEBUG
	pthread_lock(&threads_sleepers_lock);
	threads_sleepers--;
	pthread_unlock(&threads_sleepers_lock);
#endif
	restore_interrupt_enable(enabled);
	restore_preemption_enable(preemptable);

        /* grab mutex before returning */
        pthread_mutex_lock(m);

        return 0;
}

/*
 * The timed version. Time is given as an absolute time in the future.
 */
int
pthread_cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m,
		       oskit_timespec_t *abstime)
{
	pthread_thread_t		*pthread = CURPTHREAD();
	struct pthread_cond_impl	*pimpl;
	int				enabled, error = 0;
	oskit_timespec_t		now;
        struct oskit_itimerspec		ts = {{ 0, 0 }, { 0, 0 }};
	extern oskit_clock_t		*oskit_system_clock;    /* XXX */

	pthread_testcancel();

	if (c->impl == NULL && ((error = pthread_cond_init(c, NULL))))
		return error;

	save_preemption_enable(enabled);
	disable_preemption();

	pimpl = c->impl;

	/* Must block interrupts before taking condvar lock or waitlock */
	assert_interrupts_enabled();
	disable_interrupts();

	pthread_lock(&(pimpl->lock));
	
	/*
	 * Oh, this is such a dumb interface! Now must check that the abstime
	 * has not passed, which means another check of the system clock,
	 * which is what the caller just did. But since the caller might not
	 * have disabled interrupts, the time could have passed by this point.
	 */
	oskit_clock_gettime(oskit_system_clock, &now);
	if (now.tv_sec > abstime->tv_sec ||
	    (now.tv_sec == abstime->tv_sec &&
	     now.tv_nsec >= abstime->tv_nsec)) {
		error = ETIMEDOUT;
		pthread_unlock(&(pimpl->lock));
		/* unlock mutex */
		pthread_mutex_unlock(m);
		goto skip;
	}

        /* place ourself on the queue */
	queue_check(&(pimpl->waiters), pthread);
	queue_enter(&(pimpl->waiters), pthread, pthread_thread_t *, chain);

        /* unlock mutex */
        pthread_mutex_unlock(m);

	pthread_lock(&pthread->waitlock);
	pthread_unlock(&(pimpl->lock));

	/* Start the timer */
	ts.it_value.tv_sec  = abstime->tv_sec;
	ts.it_value.tv_nsec = abstime->tv_nsec;
        oskit_timer_settime(pthread->condtimer, OSKIT_TIMER_ABSTIME, &ts);

	pthread->waitflags |= THREAD_WS_CONDWAIT;
	pthread->waitcond   = c;

#ifdef  THREADS_DEBUG
	pthread_lock(&threads_sleepers_lock);
	threads_sleepers++;
	pthread_unlock(&threads_sleepers_lock);
#endif
	/* and block */
	pthread_sched_reschedule(RESCHED_BLOCK, &pthread->waitlock);

#ifdef  THREADS_DEBUG
	pthread_lock(&threads_sleepers_lock);
	threads_sleepers--;
	pthread_unlock(&threads_sleepers_lock);
#endif
	/*
	 * Look for timeout.
	 */
	pthread_lock(&pthread->waitlock);

	if (pthread->waitflags & THREAD_WS_TIMEDOUT) {
		pthread->waitflags &= ~THREAD_WS_TIMEDOUT;
		error = ETIMEDOUT;
	}
	else {
		error = 0;
		
		/* disarm the timer. */
		ts.it_value.tv_sec  = 0;
		ts.it_value.tv_nsec = 0;
		oskit_timer_settime(pthread->condtimer, 0, &ts);
	}

	pthread_unlock(&pthread->waitlock);
	
   skip:
	enable_interrupts();
	restore_preemption_enable(enabled);
	
        /* grab mutex before returning */
        pthread_mutex_lock(m);

	/* Look for a cancelation point */
	pthread_testcancel();

        return error;
}

#ifdef CPU_INHERIT
/*
 * The donating version. 
 */
int
pthread_cond_donate_wait(pthread_cond_t *c, pthread_mutex_t *m,
			 oskit_timespec_t *abstime, pthread_t donee_tid)
{
	pthread_thread_t		*pthread = CURPTHREAD();
	pthread_thread_t		*donee;
	struct pthread_cond_impl	*pimpl;
	int				enabled, error = 0;
	oskit_timespec_t		now;
        struct oskit_itimerspec		ts = {{ 0, 0 }, { 0, 0 }};
	extern oskit_clock_t		*oskit_system_clock;    /* XXX */

	pthread_testcancel();

	if (c->impl == NULL && ((error = pthread_cond_init(c, NULL))))
		return error;

	save_preemption_enable(enabled);
	disable_preemption();

	if ((donee = tidtothread(donee_tid)) == NULL_THREADPTR) {
		restore_preemption_enable(enabled);
		return EINVAL;
	}

	pimpl = c->impl;

	/* Must block interrupts before taking condvar lock or waitlock */
	assert_interrupts_enabled();
	disable_interrupts();

	pthread_lock(&(pimpl->lock));
	
	/*
	 * Oh, this is such a dumb interface! Now must check that the abstime
	 * has not passed, which means another check of the system clock,
	 * which is what the caller just did. But since the caller might not
	 * have disabled interrupts, the time could have passed by this point.
	 */
	if (abstime) {
		oskit_clock_gettime(oskit_system_clock, &now);
		if (now.tv_sec > abstime->tv_sec ||
		    (now.tv_sec == abstime->tv_sec &&
		     now.tv_nsec >= abstime->tv_nsec)) {
			error = ETIMEDOUT;
			pthread_unlock(&(pimpl->lock));
			/* unlock mutex */
			pthread_mutex_unlock(m);
			goto skip;
		}
	}

        /* place ourself on the queue */
	queue_check(&(pimpl->waiters), pthread);
	queue_enter(&(pimpl->waiters), pthread, pthread_thread_t *, chain);

        /* unlock mutex */
        pthread_mutex_unlock(m);

	pthread_lock(&pthread->waitlock);
	pthread_unlock(&(pimpl->lock));

	/* Start the timer */
	if (abstime) {
		ts.it_value.tv_sec  = abstime->tv_sec;
		ts.it_value.tv_nsec = abstime->tv_nsec;
		oskit_timer_settime(pthread->condtimer,
				    OSKIT_TIMER_ABSTIME, &ts);
	}

	pthread->waitflags |= THREAD_WS_CONDWAIT;
	pthread->waitcond   = c;

#ifdef  THREADS_DEBUG
	pthread_lock(&threads_sleepers_lock);
	threads_sleepers++;
	pthread_unlock(&threads_sleepers_lock);
#endif
	/* and donate/block */
	pthread_sched_thread_wait(donee,
				  (queue_head_t *) 0, &pthread->waitlock);
#ifdef  THREADS_DEBUG
	pthread_lock(&threads_sleepers_lock);
	threads_sleepers--;
	pthread_unlock(&threads_sleepers_lock);
#endif
	/*
	 * Look for timeout.
	 */
	if (abstime) {
		pthread_lock(&pthread->waitlock);

		if (pthread->waitflags & THREAD_WS_TIMEDOUT) {
			pthread->waitflags &= ~THREAD_WS_TIMEDOUT;
			error = ETIMEDOUT;
		}
		else {
			error = 0;
		
			/* disarm the timer. */
			ts.it_value.tv_sec  = 0;
			ts.it_value.tv_nsec = 0;
			oskit_timer_settime(pthread->condtimer, 0, &ts);
		}

		pthread_unlock(&pthread->waitlock);
	}
	
   skip:
	enable_interrupts();
	restore_preemption_enable(enabled);
	
        /* grab mutex before returning */
        pthread_mutex_lock(m);

	/* Look for a cancelation point */
	pthread_testcancel();

        return error;
}
#endif

/*      
 * Timer expiration. Note that two locks are taken at interrupt level;
 * the condition lock and the pthread waitlock. 
 */
oskit_error_t
pthread_condwait_timeout(struct oskit_iunknown *listener, void *arg)
{
	pthread_thread_t		*pthread = arg;
	pthread_cond_t			*c;
	struct pthread_cond_impl	*pimpl;
	int				enabled;

	/* Just in case some device driver renabled interrupts! */
	enabled = save_disable_interrupts();

	pthread_lock(&pthread->waitlock);
	
	DPRINTF("%p %d %p 0x%x\n",
		CURPTHREAD(), enabled, pthread, (int) pthread->waitflags);

	/*
	 * Already woken up?  Maybe from kill?
	 */
	if (! (pthread->waitflags & THREAD_WS_CONDWAIT)) {
		pthread_unlock(&pthread->waitlock);
		restore_interrupt_enable(enabled);
		return 0;
	}

	c = pthread->waitcond;
	assert(c && c->impl);
	pimpl = c->impl;

	/*
	 * Otherwise, lock the condition and remove the thread from
	 * the condition queue.
	 */
	pthread_lock(&(pimpl->lock));
	queue_remove(&(pimpl->waiters), pthread, pthread_thread_t *, chain);
	pthread_unlock(&(pimpl->lock));

	pthread->waitflags |=  THREAD_WS_TIMEDOUT;
	pthread->waitflags &= ~THREAD_WS_CONDWAIT;
	pthread_unlock(&pthread->waitlock);

	/*
	 * And place it back on the runq. Request an AST if the current
	 * thread is no longer the thread that should be running.
	 */
	if (pthread_sched_setrunnable(pthread))
		softint_request(SOFTINT_ASYNCREQ);

	DPRINTF("ends ...\n");

	restore_interrupt_enable(enabled);
	return 0;
}

/*
 * Signal a condition, waking up the highest priority thread waiting.
 */
int
pthread_cond_signal(pthread_cond_t *c)
{
	struct pthread_cond_impl	*pimpl;
	pthread_thread_t		*pnext;
	int				enabled, preemptable;

	if (c == NULL || ((pimpl = c->impl) == NULL))
		return EINVAL;

	save_preemption_enable(preemptable);
	disable_preemption();

	/* Must block interrupts before taking condvar lock or waitlock */
	enabled = save_disable_interrupts();
	
	pthread_lock(&(pimpl->lock));
	
	if (queue_empty(&(pimpl->waiters))) {
		pthread_unlock(&(pimpl->lock));
		restore_interrupt_enable(enabled);
		restore_preemption_enable(preemptable);
		return 0;
	}
	
	pnext = pthread_dequeue_fromQ(&(pimpl->waiters));
	pthread_unlock(&(pimpl->lock));

	/*
	 * Note race condition. Someone can try to cancel this thread
	 * between the time it comes off the queue, but before the state
	 * gets changed. pthread_cancel will have to deal with this.
	 */
	pthread_lock(&pnext->waitlock);
	pnext->waitflags &= ~THREAD_WS_CONDWAIT;
	pnext->waitcond = 0;
	pthread_unlock(&pnext->waitlock);
	pthread_sched_setrunnable(pnext);

	restore_interrupt_enable(enabled);
	restore_preemption_enable(preemptable);

        return 0;
}

/*
 * Wake up all threads waiting on a condition.
 */
int
pthread_cond_broadcast(pthread_cond_t *c)
{
	struct pthread_cond_impl	*pimpl;
	pthread_thread_t		*pnext;
	int				enabled, preemptable;

	if (c == NULL || ((pimpl = c->impl) == NULL))
		return EINVAL;

	save_preemption_enable(preemptable);
	disable_preemption();

	/* Must block interrupts before taking condvar lock or waitlock */
	enabled = save_disable_interrupts();

	pthread_lock(&(pimpl->lock));

	if (queue_empty(&(pimpl->waiters))) {
		pthread_unlock(&(pimpl->lock));
		restore_interrupt_enable(enabled);
		restore_preemption_enable(preemptable);
		return 0;
	}

	queue_iterate(&(pimpl->waiters), pnext, pthread_thread_t *, chain) {
		if (pnext == CURPTHREAD())
			panic("pthread_cond_broadcast: Bad queue!");

		/*
		 * Note race condition. Someone can try to cancel this
		 * thread between the time it comes off the queue, but
		 * before the state gets changed. pthread_cancel will
		 * have to deal with this.
		 */
		pthread_lock(&pnext->waitlock);
		pnext->waitflags &= ~THREAD_WS_CONDWAIT;
		pnext->waitcond   = 0;
		pthread_unlock(&pnext->waitlock);
		pthread_sched_setrunnable(pnext);
	}
	queue_init(&(pimpl->waiters));
	pthread_unlock(&(pimpl->lock));

	restore_interrupt_enable(enabled);
	restore_preemption_enable(preemptable);

        return 0;
}
