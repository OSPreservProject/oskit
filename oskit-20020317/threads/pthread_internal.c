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
 * Internal routines for threads package.
 */
#include <threads/pthread_internal.h>

/*
 * This is first routine called for every thread.
 */
void
pthread_start_thread(pthread_thread_t *pthread)
{
	/*
	 * Threads start with the current thread pointer not set yet.
	 */
	SETCURPTHREAD(pthread);

	/*
	 * All threads start with the schedlock locked.
	 */
	pthread_unlock(&pthread->schedlock);

	/*
	 * All threads start out with interrupts and preemptions blocked,
	 * which must be reset.
	 */
	enable_interrupts();
	enable_preemption();

	DPRINTF("(%d): P:%p, T:%d, F:%p A:%p\n", THISCPU,
		pthread, (int) pthread->tid, pthread->func, pthread->cookie);

	/*
	 * If the function returns a value, it is passed to pthread_exit.
	 */
	pthread_exit((*pthread->func)(pthread->cookie));

        /* NOTREACHED */
}

/*
 * The idle thread exists to spin looking for threads to run, and provide
 * a place for pthread_delay() to get called so that in usermode CPU time
 * is not wildly consumed.
 */
void *
pthread_idle_function(void *arg)
{
	DPRINTF("(%d): starting\n", THISCPU);

	/*
	 * The idle thread is run preemption disabled.
	 */
	disable_preemption();

	while (1) {
		assert_interrupts_enabled();
		assert_preemption_disabled();

		pthread_reap_threads();

		/*
		 * If nothing to schedule call the delay function
		 */
		if (!pthread_sched_reschedule(RESCHED_INTERNAL, 0))
			pthread_delay();
	}

	return 0;
}

/*
 * Unblock all the threads in the given queue.
 *
 * The caller should have locked the Q.
 */
void
pthread_resume_allQ(queue_head_t *queue)
{
	pthread_thread_t	*pnext;

	queue_iterate(queue, pnext, pthread_thread_t *, chain) {
		if (pnext == CURPTHREAD())
			panic("pthread_resume_allQ");

		pthread_sched_setrunnable(pnext);
	}

	queue_init(queue);
}

/*
 * Remove the highest priority item from the Q and return it.
 *
 * The caller should have locked the Q.
 */
pthread_thread_t *
pthread_dequeue_fromQ(queue_head_t *queue)
{
	pthread_thread_t	*pnext = 0;

#ifdef  DEFAULT_SCHEDULER
	pthread_thread_t	*ptmp;
	int			maxpri = -1;

	queue_iterate(queue, ptmp, pthread_thread_t *, chain) {
		if (ptmp == CURPTHREAD())
			panic("pthread_resume_fromQ");

		if (ptmp->priority > maxpri) {
			maxpri = ptmp->priority;
			pnext  = ptmp;
		}
	}
	queue_remove(queue, pnext, pthread_thread_t *, chain);
#else
	queue_remove_first(queue, pnext, pthread_thread_t *, chain);
#endif
	return pnext;
}

/*
 * Remove a specific thread from a Q, but only if that thread is really
 * on the queue.
 *
 * The caller should have locked the Q.
 */
pthread_thread_t *
pthread_remove_fromQ(queue_head_t *queue, pthread_thread_t *pthread)
{
	pthread_thread_t	*ptmp = NULL_THREADPTR;

	queue_iterate(queue, ptmp, pthread_thread_t *, chain) {
		if (ptmp == CURPTHREAD())
			panic("pthread_remove_fromQ");

		if (ptmp == pthread) {
			queue_remove(queue, ptmp, pthread_thread_t *, chain);
			return ptmp;
		}
	}

	return NULL_THREADPTR;
}

void
pthread_lock_panic(pthread_lock_t *lock)
{
	panic("spin_lock: "
	      "Lock 0x%x locked for a long time. Maybe you deadlocked!",
	      (int) lock);
}

#ifdef	THREADS_DEBUG
void
pthread_lock_debug(pthread_lock_t *lock)
{
#ifdef  SMP
	int		i;

	for (i = 0; i < 2000000; i++)
#endif
		if (spin_try_lock(lock))
			return;

	pthread_lock_panic(lock);
}
#endif
