/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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
 * Join with another thread. Note that the POSIX spec says that it is
 * undefined if multiple callers try to join with the same thread. If
 * that happens, things will certainly break.
 */
#include <threads/pthread_internal.h>

int
pthread_join(pthread_t tid, void **status)
{
	pthread_thread_t	*joinee, *joiner;

	if ((joinee = tidtothread(tid)) == NULL_THREADPTR)
		return EINVAL;
	
	joiner = CURPTHREAD();

	assert_preemption_enabled();
	disable_preemption();

	pthread_lock(&(joinee->lock));
	if (joinee->flags & THREAD_DETACHED) {
		pthread_unlock(&(joinee->lock));
		enable_preemption();
		return EINVAL;
	}
	pthread_unlock(&(joinee->lock));
	enable_preemption();

	/*
	 * Use a mutex here. This avoids specialized handling in the cancel
	 * and signal code. It works becase the "dead" flag is independent,
	 * protected by a spinning mutex in the reaper code. 
	 */
	pthread_mutex_lock(&joinee->mutex);
	while (!joinee->dead) {
		/*
		 * join must be called with cancelation DEFERRED!
		 */
		pthread_testcancel();

		pthread_cond_wait(&joinee->cond, &joinee->mutex);
	}

	/*
	 * Not allowed to detach the target thread if this thread is canceled.
	 */
	pthread_testcancel();

	disable_preemption();
	if (status)
		*status = (void *) joinee->exitval;

	pthread_mutex_unlock(&joinee->mutex);
	pthread_destroy_internal(joinee);
	enable_preemption();

	return 0;
}
