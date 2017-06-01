/*
 * Copyright (c) 1998, 1999 University of Utah and the Flux Group.
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
 * Internal mutex implementation.
 */
struct pthread_mutex_impl {
	spin_lock_t	mlock;		/* Actual mutex lock */
	spin_lock_t	dlock;		/* Lock for this data structure */
	queue_head_t	waiters;	/* queue for mutex */
	void	       *holder;		/* Opaque to user */
	int		count;		/* For recursive mutexes */
	int		inherit;	/* Does priority inheritance */
	int		type;		/* Mutex type. See below */
};

/*
 * Inline mutex functions.
 *
 * NOTE: These assume the spl level is set properly before being called.
 */

/*
 * Internal function. Try to lock a mutex. Spinning version.
 */
OSKIT_INLINE int
fast_mutex_spinlock(pthread_mutex_t *m)
{
	struct pthread_mutex_impl	*pimpl = m->impl;

	while (1) {
		pthread_lock(&pimpl->mlock);

		/*
		 * Try to get the lock. If successful, set owner and count.
		 */
		if (pimpl->holder == 0) {
			pimpl->holder = (void *) CURPTHREAD();		
			pimpl->count  = 1;
			pthread_unlock(&pimpl->mlock);
			return 0;
		}
		pthread_unlock(&pimpl->mlock);
	}
}

OSKIT_INLINE int
fast_mutex_lock(pthread_mutex_t *m)
{
	struct pthread_mutex_impl	*pimpl = m->impl;

	pthread_lock(&pimpl->mlock);

	/*
	 * Try to get the lock. If successful, set owner and count.
	 */
	if (pimpl->holder == 0) {
		pimpl->holder = (void *) CURPTHREAD();		
		pimpl->count  = 1;
		pthread_unlock(&pimpl->mlock);
		return 0;
	}
	pthread_unlock(&pimpl->mlock);

	return pthread_mutex_lock(m);
}

OSKIT_INLINE int
fast_mutex_unlock(pthread_mutex_t *m)
{
	struct pthread_mutex_impl	*pimpl = m->impl;

	pthread_lock(&pimpl->mlock);

	/*
	 * If the queue is empty, the mutex is free, and the lock can be
	 * released.
	 */
	if (queue_empty(&(pimpl->waiters))) {
		pimpl->holder = NULL_THREADPTR;
		pimpl->count  = 0;
		pthread_unlock(&pimpl->mlock);
		return 0;
	}

	/*
	 * Take the slow boat.
	 */
	pthread_unlock(&pimpl->mlock);
	return pthread_mutex_unlock(m);
}
