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
 * Default implementation of osenv_sleep and friends.
 */ 

#include <threads/pthread_internal.h>
#include <oskit/debug.h>
#undef dump_stack_trace

#ifndef KNIT
void
pthread_init_osenv_sleep()
{
	/*
	 * A symbol to make sure this is picked up from the pthreads library.
	 */
}
#endif

void
osenv_sleep_init(osenv_sleeprec_t *sr) 
{
	pthread_thread_t	*pthread = CURPTHREAD();
	
	osenv_assert(sr);

	sr->data[0] = OSENV_SLEEP_WAKEUP;
	sr->data[1] = pthread;
}

int
osenv_sleep(osenv_sleeprec_t *sr) 
{
	volatile osenv_sleeprec_t *vsr      = sr;
	int			  enabled;
	pthread_thread_t	  *pthread;

	osenv_assert(sr);
	osenv_assert(CURPTHREAD() != IDLETHREAD);

	enabled = save_disable_interrupts();

	if ((pthread = (pthread_thread_t *) sr->data[1]) == NULL_THREADPTR) {
		restore_interrupt_enable(enabled);
		return (int) vsr->data[0];
	}

	osenv_assert(CURPTHREAD() == pthread);
	
	if (pthread)
		DPRINTF("1: %p %d\n", pthread, (int) pthread->tid);

	/*
	 * Check for cancelation before going into the sleep.
	 */
	pthread_lock(&pthread->lock);
	if (pthread->flags & THREAD_CANCELED) {
		pthread_unlock(&pthread->lock);
		restore_interrupt_enable(enabled);
		return OSENV_SLEEP_CANCELED;
	}
	pthread_unlock(&pthread->lock);

	/*
	 * Save sleeprec. Need this stuff for cancelation and signals.
	 */
	pthread->sleeprec = sr;

	while (1) {
		if (vsr->data[1] == NULL)
			break;

		osenv_process_release();

#ifdef  THREADS_DEBUG
		pthread_lock(&threads_sleepers_lock);
		threads_sleepers++;
		pthread_unlock(&threads_sleepers_lock);
#endif
		pthread_lock(&pthread->waitlock);
		pthread->waitflags |= THREAD_WS_OSENVSLEEP;
		pthread_sched_reschedule(RESCHED_BLOCK, &pthread->waitlock);

#ifdef  THREADS_DEBUG
		pthread_lock(&threads_sleepers_lock);
		threads_sleepers--;
		pthread_unlock(&threads_sleepers_lock);
#endif
		osenv_process_lock();
	}
	pthread->sleeprec = 0;

	DPRINTF("2: %p %d\n", pthread, (int) pthread->tid);

	restore_interrupt_enable(enabled);
	return (int) vsr->data[0];
}

/*
 * This clears the condition that osenv_sleep is blocked on and
 * wakes up the thread blocked on it.
 */
void
osenv_wakeup(osenv_sleeprec_t *sr, int wakeup_status)
{
	pthread_thread_t	*pthread;
	int			resched, enabled;
	
	enabled = save_disable_interrupts();

	/* Return immediately on spurious wakeup */
	if ((pthread = (pthread_thread_t *) sr->data[1]) == NULL_THREADPTR) {
		restore_interrupt_enable(enabled);
		return;
	}

	DPRINTF("%p(%d) status:%d\n",
		pthread, (int) pthread->tid, wakeup_status);

	pthread_lock(&pthread->waitlock);
	pthread->waitflags &= ~THREAD_WS_OSENVSLEEP;

	sr->data[0] = (void *)wakeup_status;
	sr->data[1] = (void *)0;

	pthread_unlock(&pthread->waitlock);

	/*
	 * And get the thread scheduled again ...
	 */
	resched = pthread_sched_setrunnable(pthread);
	
	if (resched)
		softint_request(SOFTINT_ASYNCREQ);

	restore_interrupt_enable(enabled);
}
