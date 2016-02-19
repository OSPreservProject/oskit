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

/*
 * Exit code.
 */
#include <threads/pthread_internal.h>
#include "pthread_mutex.h"
#ifdef PRI_INHERIT
#include "sched_posix.h"
#endif

static pthread_thread_t *last_dead_thread;
static pthread_lock_t	reaper_lock;

/*
 * Just a dummy lock to satisfy the switch code.
 */
pthread_lock_t		threads_exit_lock    = PTHREAD_LOCK_INITIALIZER;

extern int		threads_alive;

void
pthread_exit(void *status)
{
	pthread_thread_t  *pthread = CURPTHREAD();

	assert_interrupts_enabled();
	assert_preemption_enabled();
	disable_preemption();
	
	pthread_lock(&pthread->lock);

	pthread_exit_locked(status);

	/*
	 * Never returns.
	 */
}

/*
 * Internal version. Thread lock is locked.
 */
void
pthread_exit_locked(void *status)
{
	pthread_thread_t  *pthread = CURPTHREAD();
	int		  detached;

	DPRINTF("(%d): %p(%p) %p\n", THISCPU, pthread, pthread->tid, status);

	if (pthread->flags & THREAD_EXITING) {
		panic("pthread_exit: Recursive call: 0x%x(%p)\n",
		      (int) pthread, pthread->tid);
	}

	detached         = pthread->flags & THREAD_DETACHED;
	pthread->flags  |= THREAD_EXITING;
	pthread->exitval = (oskit_u32_t) status;

	/*
	 * The standard says that cancellation is disabled while the handlers
	 * run.  Do that now before we unlock/reenable everything.
	 * Note that we just leave cancellation disabled even after the
	 * handlers return.
	 */
	pthread->cancelstate = PTHREAD_CANCEL_DISABLE;

	pthread_unlock(&pthread->lock);

	/*
	 * Enable preemption and interrupts while the cleanups and
	 * destructors are run, in case they misbehave.
	 */
	enable_interrupts();
	enable_preemption();

	/*
	 * Call the cancelation handlers.
	 */
	pthread_call_cleanup_handlers();
		
	/*
	 * Call the key destructors.
	 */
	pthread_call_key_destructors();

#ifdef  PRI_INHERIT
	/*
	 * If this thread is inheriting from a thread (which means a thread
	 * or threads is waiting on it), its probably a bad thing cause
	 * some resource is locked up.
	 */
	if (! queue_empty(&pthread->waiters))
		printf("PTHREAD_EXIT: TID(%p) is exiting, but other "
		       "threads are waiting for it. DEADLOCK?\n",
		       pthread->tid);
#endif
	if (--threads_alive == 0)
		exit(0);

	/*
	 * Let the idle thread clean up the thread if its detached since
	 * we cannot reap ourselves. Note that the detached flag was
	 * checked above, and then the lock released. This is okay since
	 * the program needs to arrange its own synchronization between
	 * thread creation and detach/join.
	 */
	disable_preemption();
	if (detached) {
		pthread_lock(&reaper_lock);
		while (last_dead_thread) {
			pthread_thread_t	*dead_thread;

			/*
			 * Clear last_dead_thread and release the reaper
			 * lock so that it will not be held during the
			 * thread destruction (memory deallocation might
			 * block). The idle thread checks last_dead_thread
			 * too, so it must be cleared before the lock is
			 * released.
			 */
			dead_thread      = last_dead_thread;
			last_dead_thread = NULL_THREADPTR;
			pthread_unlock(&reaper_lock);
			
			pthread_lock(&dead_thread->lock);
			pthread_destroy_internal(dead_thread);

			/*
			 * Reacquire the lock before continuing.
			 */
			pthread_lock(&reaper_lock);
		}
		last_dead_thread = pthread;
		pthread_sched_reschedule(RESCHED_BLOCK, &reaper_lock);
	}
	else {
		/*
		 * Not detached, but perhaps someone is waiting for it.
		 * Wake up the joiner thread and let it reap us.
		 */
		pthread_mutex_lock(&pthread->mutex);
		pthread->dead = 1;
		pthread_cond_signal(&pthread->cond);
		pthread_mutex_unlock(&pthread->mutex);
		pthread_sched_reschedule(RESCHED_BLOCK, NULL);
	}
	
	/*
	 * Never returns.
	 */
	panic("pthread_exit: dead thread returns!");
}

/*
 * Called from the idle loop each time through to look for dead threads.
 */
void
pthread_reap_threads(void)
{
	pthread_thread_t	*pthread;
	
	pthread_lock(&reaper_lock);
	while (last_dead_thread) {
		/*
		 * Clear last_dead_thread and release the reaper
		 * lock so that it will not be held during the
		 * thread destruction (memory deallocation might
		 * block). 
		 */
		pthread = last_dead_thread;
		last_dead_thread = NULL_THREADPTR;
		pthread_unlock(&reaper_lock);
		
		pthread_lock(&pthread->lock);
		assert(pthread->flags & THREAD_DETACHED);
		pthread_destroy_internal(pthread);

		pthread_lock(&reaper_lock);
	}
	pthread_unlock(&reaper_lock);
}

void
pthread_init_exit()
{
	pthread_lock_init(&reaper_lock);
}
