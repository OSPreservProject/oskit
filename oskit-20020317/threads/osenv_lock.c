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
 * Process locks
 */
#include <threads/pthread_internal.h>

pthread_mutex_t		threads_process_lock;
pthread_thread_t       *threads_process_lock_owner;

/*
 * Lock the process lock.
 */
void
osenv_process_lock(void)
{
	pthread_thread_t	*pthread = CURPTHREAD();

	if (threads_process_lock_owner == pthread)
		panic("osenv_process_lock: "
		      "0x%x(%p)\n", (int) pthread, pthread->tid);

	pthread_mutex_lock(&threads_process_lock);
	threads_process_lock_owner = CURPTHREAD();
}

/*
 * Release the process lock. Called from osenv_sleep(), and it differs
 * in that it does not check for a killed thread. That won't happen
 * until the unlock is done.
 */
void
osenv_process_release(void)
{
	pthread_thread_t	*pthread = CURPTHREAD();

	if (threads_process_lock_owner != pthread) {
		if (threads_process_lock_owner != NULL_THREADPTR)
			printf("osenv_process_release: owner: 0x%x(%p)\n",
			       (int) threads_process_lock_owner,
			       threads_process_lock_owner->tid);
		panic("osenv_process_release: current: "
		      "0x%x(%p)\n", (int) pthread, pthread->tid);
	}

	threads_process_lock_owner = NULL_THREADPTR;
	pthread_mutex_unlock(&threads_process_lock);
}

/*
 * Unlock the process lock, checking for a dead thread that needs to be
 * reaped.
 */
void
osenv_process_unlock(void)
{
	pthread_thread_t	*pthread = CURPTHREAD();

	if (threads_process_lock_owner != pthread) {
		if (threads_process_lock_owner != NULL_THREADPTR)
			printf("osenv_process_unlock: owner: 0x%x(%p)\n",
			       (int) threads_process_lock_owner,
			       threads_process_lock_owner->tid);
		
		panic("osenv_process_unlock: current: "
		      "0x%x(%d)\n", (int) pthread, pthread->tid);
	}

	threads_process_lock_owner = NULL_THREADPTR;
	pthread_mutex_unlock(&threads_process_lock);

	if (pthread->flags & THREAD_KILLED) {
		pthread_exit((void *) PTHREAD_CANCELED);

		/*
		 * Never returns.
		 */
	}
}

/*
 * Check the process lock.
 */
void
osenv_process_lock_check(void)
{
	pthread_thread_t	*pthread = CURPTHREAD();

	if (threads_process_lock_owner != pthread)
		panic("osenv_process_lock: "
		      "0x%x(%d)\n", (int) pthread, pthread->tid);
}

/*
 * Test if the current thread has the process lock. Kludge for usermode.
 */
int
osenv_process_locked(void)
{
	return threads_process_lock_owner == CURPTHREAD();
}

void
pthread_init_process_lock(void)
{
	pthread_mutex_init(&threads_process_lock, 0);
}
