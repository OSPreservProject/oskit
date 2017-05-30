/*
 * Copyright (c) 1998, 1999, 2000, 2001 University of Utah and the Flux Group.
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
#include "pthread_mutex.h"
#ifdef PRI_INHERIT
#include "sched_posix.h"
#endif

/* Protect static initialization against race. */
static pthread_lock_t static_lock = PTHREAD_LOCK_INITIALIZER;

/*
 * Create and initialize a new mutex. For now, I am going to allow
 * a mutex to be reinitialized before being destroyed. This is certainly
 * a violation of the spec.
 */
int
pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *attr)
{
	struct pthread_mutex_impl	*pimpl;
	
	if (m == NULL)
		return EINVAL;
	
	if (! attr)
		attr = &pthread_mutexattr_default;

	if ((pimpl = (struct pthread_mutex_impl *)
	     smalloc(sizeof(*pimpl))) == NULL)
		return ENOMEM;
	m->impl = pimpl;

        queue_init(&(pimpl->waiters));
	pthread_lock_init(&(pimpl->mlock));
	pthread_lock_init(&(pimpl->dlock));
	pimpl->holder  = 0;
	pimpl->inherit = 0;
	pimpl->count   = 0;
	pimpl->type    = attr->type;
	pimpl->inherit = attr->priority_inherit;

        return 0;
}

/*
 * Destroy a mutex by deallocating the implementation.
 */
int
pthread_mutex_destroy(pthread_mutex_t *m)
{
	struct pthread_mutex_impl	*pimpl;
	int				enabled;

	if (m == NULL || ((pimpl = m->impl) == NULL))
		return EINVAL;

	save_preemption_enable(enabled);
	disable_preemption();

	pthread_lock(&pimpl->mlock);

	/*
	 * If the mutex can be locked, its safe to destroy it.
	 */
	if (pimpl->holder == 0) {
		sfree(pimpl, sizeof(*pimpl));
		m->impl = NULL;
		restore_preemption_enable(enabled);
		return 0;
	}

	pthread_unlock(&pimpl->mlock);
	restore_preemption_enable(enabled);
	return EBUSY;
}

/*
 * Static initializer. When an application declares a static mutex,
 * it is really just a pointer to NULL. Allocate an actual mutex
 * object, 
 */
int
pthread_mutex_init_static(pthread_mutex_t *m)
{
	int	enabled, rc = 0;

	save_preemption_enable(enabled);
	disable_preemption();
	
	pthread_lock(&static_lock);

	if (m && m->impl == NULL)
		rc = pthread_mutex_init(m, NULL);

	pthread_unlock(&static_lock);
	restore_preemption_enable(enabled);

	return rc;
}

/*
 * Try to lock a mutex. Non-blocking variant.
 */
int
pthread_mutex_trylock(pthread_mutex_t *m)
{
	struct pthread_mutex_impl	*pimpl;
	int				rc, enabled;

	if (m->impl == NULL && ((rc = pthread_mutex_init_static(m))))
		return rc;

	pimpl = m->impl;

	save_preemption_enable(enabled);
	disable_preemption();
	
	pthread_lock(&pimpl->mlock);

	/*
	 * Try to get the mutex. If successful, set owner and count.
	 */
	if (pimpl->holder == 0) {
		pimpl->holder = (void *) CURPTHREAD();		
		pimpl->count  = 1;
		pthread_unlock(&pimpl->mlock);
		restore_preemption_enable(enabled);
		return 0;
	}
	pthread_unlock(&pimpl->mlock);
	restore_preemption_enable(enabled);
	return EBUSY;
}

/*
 * Try to lock a mutex. Blocking variant.
 */
int
pthread_mutex_lock(pthread_mutex_t *m)
{
	pthread_thread_t		*pthread = CURPTHREAD();
	struct pthread_mutex_impl	*pimpl;
	int				rc, enabled;

	if (m->impl == NULL && ((rc = pthread_mutex_init_static(m))))
		return rc;

	pimpl = m->impl;

	save_preemption_enable(enabled);
	disable_preemption();

	pthread_lock(&pimpl->mlock);

	/*
	 * Try to get the lock. If successful, set owner and count.
	 */
	if (pimpl->holder == 0) {
		pimpl->holder = (void *) CURPTHREAD();		
		pimpl->count  = 1;
		pthread_unlock(&pimpl->mlock);
		restore_preemption_enable(enabled);
		return 0;
	}

	/*
	 * Can't get it. Look for a recursive lock. 
	 */
	if (pimpl->holder == CURPTHREAD()) {
		if (pimpl->type == PTHREAD_MUTEX_RECURSIVE) {
			pimpl->count++;
			pthread_unlock(&pimpl->mlock);
			restore_preemption_enable(enabled);
			return 0;
		}
		else
			pthread_mutex_panic(m, "lock", "recursive attempt");
	}

#ifdef  CPU_INHERIT
  again:
#endif
	/*
	 * Okay, time to block ...
	 */
	queue_check(&(pimpl->waiters), pthread);
	
#ifdef	PRI_INHERIT
	if (pimpl->inherit)
		pthread_priority_inherit(pimpl->holder);
#endif
#ifdef  CPU_INHERIT
	if (pimpl->inherit) {
		pthread_sched_thread_wait(pimpl->holder,
					  &(pimpl->waiters), &(pimpl->mlock));

		/*
		 * All threads are woken up to undo the donation.
		 */
		if (pimpl->holder != CURPTHREAD()) {
			/*printf("pthread_mutex_lock: "
			       "0x%x 0x%x(%p) 0x%x(%p)\n",
			       (int) m, (int) pimpl->holder,
			       (int) ((pthread_thread_t *)pimpl->holder)->tid,
			       (int) CURPTHREAD(), CURPTHREAD()->tid); */
			pthread_lock(&pimpl->mlock);
			goto again;
		}
	}
	else
#endif
#ifdef  SIMPLE_PRI_INHERIT
	if (pimpl->inherit) {
		queue_head_t		*q = &(pimpl->waiters);
		pthread_thread_t	*ptmp;
		
		if (queue_empty(q)) {
			queue_enter(q, pthread, pthread_thread_t *, chain);
		}
		else {
			/*
			 * Insert into queue in priority order. Highest
			 * priority waiter at head of queue so it gets
			 * the mutex next when unlocked.
			 */
			queue_iterate(q, ptmp, pthread_thread_t *, chain) {
				if (ptmp == CURPTHREAD())
					pthread_mutex_panic(m, "lock",
							    "Bad Queue");

				if (pthread->priority > ptmp->priority) {
					queue_enter_before(q, ptmp, pthread,
							   pthread_thread_t *,
							   chain);
					goto inserted;
				}
			}
			queue_enter(q, pthread, pthread_thread_t *, chain);
		inserted:
		}
		/*
		 * Okay, head of waiters queue is the highest priority thread
		 * waiting to get the mutex. If its priority is greater than
		 * the thread holding the mutex, then we want its priority
		 * transfered to the thread holding the mutex. As it turns out,
		 * if the current thread is not the first on the list, then
		 * the thread holding the mutex already has already had it
		 * priority bumped.
		 *
		 * Anywaym let the scheduler do the rest. When we return, we 
		 * have the mutex and the priority donation has been undone. 
		 */
		pthread_sched_priority_transfer_and_wait(pimpl->holder,
							 &(pimpl->mlock));
	}
	else
#endif
	{
	queue_enter(&(pimpl->waiters), pthread, pthread_thread_t *, chain);
	pthread_sched_reschedule(RESCHED_BLOCK, &(pimpl->mlock));
	}

	/*
	 * Unlock assures that this thread wakes up owning the mutex.
	 */
	if (pimpl->holder != CURPTHREAD())
		pthread_mutex_panic(m, "lock", "Not the owner");
	
	restore_preemption_enable(enabled);
        return 0;
}

/*
 * Unlock a mutex.
 */
int
pthread_mutex_unlock(pthread_mutex_t *m)
{
	pthread_thread_t		*pnext = NULL_THREADPTR;
	struct pthread_mutex_impl	*pimpl;
	int				enabled;

	if (m == NULL || ((pimpl = m->impl) == NULL))
		return EINVAL;

	/* Can be called from condition code and osenv_unlock. */
	save_preemption_enable(enabled);
	disable_preemption();

	pthread_lock(&pimpl->mlock);

	if (pimpl->holder != CURPTHREAD()) {
		/*
		 * This panic should be under mutex type control ...
		 */
		pthread_mutex_panic(m, "unlock", "wrong owner");
	}

	/*
	 * Decrement the count and look for a possible recursive lock.
	 */
	if (--pimpl->count != 0) {
		if (pimpl->count < 0)
			pthread_mutex_panic(m, "unlock", "negative count");
		
		if (pimpl->type == PTHREAD_MUTEX_RECURSIVE) {
			pthread_unlock(&pimpl->mlock);
			restore_preemption_enable(enabled);
			return 0;
		}

		pthread_mutex_panic(m, "unlock", "non-zero count");
	}

	/*
	 * If the queue is empty, the mutex is free, and the lock can be
	 * released. If the queue is non-empty, remove one and transfer
	 * ownership.
	 */
	if (queue_empty(&(pimpl->waiters))) {
		pimpl->holder = NULL_THREADPTR;
		pimpl->count  = 0;
		pthread_unlock(&pimpl->mlock);
		restore_preemption_enable(enabled);
		return 0;
	}

	/*
	 * Grant to next thread directly.
	 */
#ifdef  SIMPLE_PRI_INHERIT
	if (pimpl->inherit) {
		queue_remove_first(&(pimpl->waiters),
				   pnext, pthread_thread_t *, chain);
		pthread_sched_priority_transfer_undo(CURPTHREAD());
	}
	else
#endif
	pnext = pthread_dequeue_fromQ(&(pimpl->waiters));

	pimpl->holder = pnext;
	pimpl->count  = 1;
	pthread_sched_setrunnable(pnext);
#ifdef  PRI_INHERIT
	if (pimpl->inherit)
		pthread_priority_uninherit(pnext);
#endif
#ifdef  CPU_INHERIT
	/*
	 * Wake up all of the other threads to undo donation.
	 */
	if (pimpl->inherit)
		pthread_resume_allQ(&(pimpl->waiters));
#endif
	pthread_unlock(&(pimpl->mlock));
	restore_preemption_enable(enabled);
        return 0;
}

void
pthread_mutex_panic(pthread_mutex_t *m, char *type, char *msg)
{
	struct pthread_mutex_impl	*pimpl = m->impl;
	pthread_thread_t		*pholder = pimpl->holder;
	
	printf("pthread_mutex_%s: %s - %p/%p\n", type, msg, m, pimpl);

	if (pholder && pholder != CURPTHREAD()) {
		printf("Backtrace for owner 0x%x(%p)\n",
		       (int) pholder, pholder->tid);
		threads_stack_back_trace((int)pholder->tid, 16);
	}
	panic("pthread_mutex_panic");
}
