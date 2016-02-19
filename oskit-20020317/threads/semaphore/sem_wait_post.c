/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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

#include "sem.h"

/*
 *  Lock a Semaphore
 */
int
oskit_sem_wait(sem_t *sem)
{
	int enabled;
	int ret = 0;

	pthread_testcancel();
	save_preemption_enable(enabled);
	disable_preemption();
	pthread_lock(&sem->sem_lock);

	if (--(sem->sem_value) < 0) {
		pthread_thread_t *pthread = CURPTHREAD();
		/* make block myself */
		queue_check(&sem->sem_waiters, pthread);
		queue_enter(&sem->sem_waiters, pthread, 
			    pthread_thread_t *, chain);
		pthread_lock(&pthread->waitlock);
		pthread_unlock(&sem->sem_lock);
		pthread->waitflags |= THREAD_WS_SEMWAIT;
		pthread_sched_reschedule(RESCHED_BLOCK, &pthread->waitlock);
		pthread_lock(&pthread->waitlock);
		pthread->waitflags &= ~THREAD_WS_SEMWAIT;
		pthread_unlock(&pthread->waitlock);
		restore_preemption_enable(enabled);
		pthread_testcancel();
	} else {
		pthread_unlock(&sem->sem_lock);
		restore_preemption_enable(enabled);
	}

	return ret;
}

/* called from pthread_cancel() */
void
oskit_sem_wait_cancel(pthread_thread_t *pthread)
{
	if (pthread->waitflags & THREAD_WS_SEMWAIT) {
		pthread->waitflags &= ~THREAD_WS_SEMWAIT;
		pthread_unlock(&pthread->waitlock);
		pthread_sched_setrunnable(pthread);
	} else {
		panic("oskit_semwait_cancel");
	}
}


int
oskit_sem_trywait(sem_t *sem)
{
	int enabled;
	int ret = 0;

	save_preemption_enable(enabled);
	disable_preemption();
	pthread_lock(&sem->sem_lock);

	if (sem->sem_value >= 1) {
		sem->sem_value--;
	} else {
		ret = EAGAIN;
	}
	pthread_unlock(&sem->sem_lock);
	restore_preemption_enable(enabled);
	return ret;
}

/*
 *  Unlock a Semaphore
 */
int
oskit_sem_post(sem_t *sem)
{
	pthread_thread_t *pnext;
	int enabled;
	int ret = 0;

	save_preemption_enable(enabled);
	disable_preemption();
	pthread_lock(&sem->sem_lock);

	if (++(sem->sem_value) <= 0) {
		/* wakeup someone blocking */
		pnext = pthread_dequeue_fromQ(&sem->sem_waiters);
		pthread_sched_setrunnable(pnext);
	} else {
#if 0
		if (!queue_empty(&sem->sem_waiters)) {
			panic("oskit_sem_post: %d\n", sem->sem_value);
		}
#endif
	}

	pthread_unlock(&sem->sem_lock);
	restore_preemption_enable(enabled);
	return ret;
}

