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
 * Detach from the parent.
 */
#include <threads/pthread_internal.h>

int
pthread_detach(pthread_t tid)
{
	pthread_thread_t	*pthread;

	if ((pthread = tidtothread(tid)) == NULL_THREADPTR)
		return EINVAL;

	assert_preemption_enabled();
	disable_preemption();

	pthread_lock(&pthread->lock);
	pthread->flags |= THREAD_DETACHED;
	pthread_unlock(&pthread->lock);

	pthread_mutex_lock(&pthread->mutex);
	if (pthread->dead) {
		pthread_mutex_unlock(&pthread->mutex);
		pthread_destroy_internal(pthread);
	}
	pthread_mutex_unlock(&pthread->mutex);

	enable_preemption();

	return 0;
}
