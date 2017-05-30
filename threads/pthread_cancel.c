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
 * Cancel Code.
 */
#include <threads/pthread_internal.h>
#include <threads/pthread_ipc.h>
#include <oskit/c/malloc.h>
#include "pthread_cond.h"
#ifdef PRI_INHERIT
#include "sched_posix.h"
#endif

int
pthread_cancel(pthread_t tid)
{
	pthread_thread_t  *pthread;

	assert_preemption_enabled();
	assert_interrupts_enabled();

	if ((pthread = tidtothread(tid)) == NULL_THREADPTR)
		return EINVAL;

	if (CURPTHREAD()->tid == tid) {
		pthread_exit((void *) -1);
		return 0;
	}

	disable_preemption();
	pthread_lock(&pthread->lock);

	if (pthread->flags & (THREAD_EXITING|THREAD_CANCELED)) {
		pthread_unlock(&pthread->lock);
		enable_preemption();
		return 0;
	}
	pthread->flags |= THREAD_CANCELED;

	/*
	 * Test cancel state. If disabled it must be deferred until
	 * cancelation is enabled.
	 */
	if (pthread->cancelstate == PTHREAD_CANCEL_DISABLE) {
		pthread_unlock(&pthread->lock);
		enable_preemption();
		return 0;
	}

	/*
	 * Test cancel type. In either case (ASYNC or DEFERRED), the thread
	 * is woken is up. The difference is that ASYNC forces the thread
	 * into pthread_exit immediately upon being switched back in, while
	 * DEFERRED allows the thread to continue executing until it
	 * reaches a cancelation point.  So, pthread_testcancel just calls
	 * pthread_exit if the cancelpending is set.
	 */
	if (pthread->canceltype != PTHREAD_CANCEL_DEFERRED)
		pthread->flags |= THREAD_KILLED;

#ifdef  PRI_INHERIT
	/*
	 * If this thread is inheriting from a thread (which means a thread
	 * or threads is waiting on it), its probably a bad thing cause
	 * some resource is locked up.
	 */
	if (! queue_empty(&pthread->waiters))
		printf("PTHREAD_CANCEL: TID(%p) was canceled, but other "
		       "threads are waiting for it. DEADLOCK?\n",
		       pthread->tid);

	/*
	 * If instead this thread is donating its priority to another
	 * thread that holds a resource, just undo the inheritance.
	 */
	if (pthread->waiting_for)
		pthread_priority_kill(pthread);
#endif
	
	/*
	 * All kinds of conditions are possible at this point.
	 */

	/*
	 * Below here, we muck with the waitflags, which are accessed
	 * from interrupt handlers. Must block interrupts and take
	 * that lock first.
	 */
	disable_interrupts();
	pthread_unlock(&pthread->lock);
	pthread_lock(&pthread->waitlock);

#ifdef  CPU_INHERIT
	/*
	 * CPUI recv wait.
	 */
	if (pthread->waitflags & THREAD_WS_CPUIRECV_WAIT) {
		pthread_sched_recv_cancel(pthread);
		goto done;
	}
#endif	

	/*
	 * IPC Wait.
	 */
	if (pthread->waitflags & THREAD_WS_IPCWAIT_FLAG) {
		pthread_ipc_cancel(pthread);
		enable_interrupts();
		goto done;
	}
	
	/*
	 * If the thread is in an osenv_sleep(), issue a wakeup.
	 * The thread will be allowed to return through the sleep, to
	 * be caught sometime later. This allows driver state to be
	 * cleaned up before the thread is actually killed off.
	 */
	if (pthread->waitflags & THREAD_WS_OSENVSLEEP) {
		pthread_unlock(&pthread->waitlock);
		osenv_wakeup(pthread->sleeprec, OSENV_SLEEP_CANCELED);
		enable_interrupts();
		goto done;
	}

	/*
	 * If the thread is THREAD_WS_SLEEPING, then restart it. The death 
	 * will be noticed before the thread is allowed to return from 
	 * the call.
	 */
	if (pthread->waitflags & THREAD_WS_SLEEPING) {
		pthread_wakeup_locked(pthread);
		enable_interrupts();
		goto done;
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
			pthread->waitcond  = 0;
			pthread_unlock(&(pimpl->lock));
			pthread_unlock(&pthread->waitlock);
			enable_interrupts();
			pthread_sched_setrunnable(pthread);
			goto done;
		}

		pthread_unlock(&(pimpl->lock));
		pthread_unlock(&pthread->waitlock);
		enable_interrupts();
		goto done;
	}

	/*
	 * Semaphore Wait.
	 */
	if (pthread->waitflags & THREAD_WS_SEMWAIT) {
		oskit_sem_wait_cancel(pthread);
		enable_interrupts();
		goto done;
	}

	pthread_unlock(&pthread->waitlock);
	enable_interrupts();

	/*
	 * Must be running on another CPU. Must wait for it to be noticed.
	 */
   done:
	enable_preemption();
	return 0;
}

int
pthread_setcancelstate(int state, int *oldstate)
{
	pthread_thread_t	*pthread = CURPTHREAD();

	if (state != PTHREAD_CANCEL_ENABLE &&
	    state != PTHREAD_CANCEL_DISABLE)
		return EINVAL;

	assert_preemption_enabled();

	disable_preemption();
	pthread_lock(&pthread->lock);
	if (oldstate) {
		*oldstate = pthread->cancelstate;
	}
	pthread->cancelstate = state;
	pthread_unlock(&pthread->lock);
	enable_preemption();
	
	return 0;
}

int
pthread_setcanceltype(int type, int *oldtype)
{
	pthread_thread_t	*pthread = CURPTHREAD();

	if (type != PTHREAD_CANCEL_DEFERRED &&
	    type != PTHREAD_CANCEL_ASYNCHRONOUS)
		return EINVAL;

	assert_preemption_enabled();

	disable_preemption();
	pthread_lock(&pthread->lock);
	if (oldtype) {
		*oldtype = pthread->canceltype;
	}
	pthread->canceltype = type;
	pthread_unlock(&pthread->lock);
	enable_preemption();
	
	return 0;
}

void
pthread_testcancel(void)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	extern int		threads_initialized;

	/*
	 * pthread_testcancel() might be called before the thread library
	 * is initialized.
	 */
	if (!threads_initialized) {
		return;
	}

	assert_preemption_enabled();

	disable_preemption();
	pthread_lock(&pthread->lock);

	if (pthread->cancelstate == PTHREAD_CANCEL_ENABLE &&
	    pthread->flags & THREAD_CANCELED) {
		pthread_exit_locked((void *) PTHREAD_CANCELED);

		/*
		 * Never returns.
		 */
	}

	pthread_unlock(&pthread->lock);
	enable_preemption();
}

/*
 * Push a cleanup handler. This is the internal version that is called
 * from the pthread_cleanup_push macro. 
 */
void
oskit_pthread_cleanup_push(pthread_cleanup_t *pcleanup,
			   void (*routine)(void *), void *arg)
{
	pthread_thread_t	*pthread = CURPTHREAD();

	assert_preemption_enabled();

	pcleanup->func = routine;
	pcleanup->arg  = arg;

	/*
	 * Link it into the chain. This maintains a backwards chain
	 * through the stack of cleanup handlers.
	 */
	pcleanup->prev    = pthread->cleanups;
	pthread->cleanups = pcleanup;
}

/*
 * Pop a cleanup handler, possibly executing it.
 */
void
oskit_pthread_cleanup_pop(pthread_cleanup_t *pcleanup, int execute)
{
	pthread_thread_t	*pthread = CURPTHREAD();

	assert_preemption_enabled();

	if (pcleanup != pthread->cleanups) {
		pthread_lock(&pthread->lock);
		panic("oskit_pthread_cleanup_pop: Mismatch!");
	}
	
	pthread->cleanups = pcleanup->prev;

	if (execute)
		pcleanup->func(pcleanup->arg);
}

/*
 * Call the cleanup handlers. Called from pthread_exit at termination.
 */
void
pthread_call_cleanup_handlers()
{
	pthread_thread_t	*pthread = CURPTHREAD();

	/*
	 * Don't have to lock the thread for accessing the cleanups, since
	 * only the current thread can muck with them.
	 */
	while (pthread->cleanups)
		oskit_pthread_cleanup_pop(pthread->cleanups, 1);
}
