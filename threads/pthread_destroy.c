/*
 * Copyright (c) 1996-2001 University of Utah and the Flux Group.
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
 * Destroy a thread.
 */
#include <threads/pthread_internal.h>
#include <threads/pthread_ipc.h>

/* 
 * Thread destroy hook.
 */
static oskit_pthread_destroy_hook_t	threads_destroy_hook;

/*
 * Internal routine. 
 */
void
pthread_destroy_internal(pthread_thread_t *pthread)
{
	pthread_t	tid = pthread->tid;
	int		rc;

	assert_preemption_disabled();

	/* Call the thread destroy hook. */
	if (threads_destroy_hook) {
	    (*threads_destroy_hook)(pthread->tid);
	}

	/*
	 * The main thread is mostly statically allocated, so just
	 * release the tid but leave everything else alone.
	 */
	if (pthread == &threads_mainthread) {
		threads_tidtothread[(int) tid] = 0;
		return;
	}

	/* machine dependent destroy */
	thread_destroy(pthread);
	
	/*
	 * Callout for memory deallocator. 
	 */
	if (pthread->pstk && (! (pthread->flags & THREAD_USERSTACK)))
		threads_deallocator(pthread->pstk);

#ifdef	CPU_INHERIT
	if (pthread->scheduler) {
		schedmsg_t	msg;

		msg.type = MSG_SCHED_EXITED;
		msg.tid  = pthread->tid;
		pthread_sched_message_send(pthread->scheduler, &msg);
	}
#endif
	oskit_timer_release(pthread->condtimer);
	oskit_timer_release(pthread->sleeptimer);
	
	rc = pthread_cond_destroy(&pthread->cond);
	assert(rc == 0);
	rc = pthread_mutex_destroy(&pthread->mutex);
	assert(rc == 0);

#ifdef THREAD_STATS
	{
		extern void save_thread_stats(pthread_thread_t *pthread);
		save_thread_stats(pthread);
	}
#endif

	threads_deallocator(pthread);
	
	/*
	 * Release the TID.
	 */
	threads_tidtothread[(int) tid] = 0;
}

/*
 * Install the thread destroy hook.  Only one hook can be set.
 * Returns previous hook function.
 */
oskit_pthread_destroy_hook_t
oskit_pthread_destroy_hook_set(oskit_pthread_destroy_hook_t hook)

{
    oskit_pthread_destroy_hook_t ohook = threads_destroy_hook;
    threads_destroy_hook = hook;
    return ohook;
}
