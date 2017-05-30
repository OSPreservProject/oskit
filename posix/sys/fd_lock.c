/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Handles details of read/write locking file descriptors
 */
#ifdef	THREAD_SAFE
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "fd.h"

#define SPINLOCK(x) ({			\
	while (! spin_try_lock((x)))	\
		sched_yield();		\
})

#define SPINUNLOCK(x) ({		\
	spin_unlock((x));		\
})

/*
 * Very simple model right now. An fd can be locked for read and/or
 * write access. This allows two threads to concurrently read/write,
 * but not read/read or write/write. Locks are also recursive.
 *
 * The FD lock must already be held!
 *
 * This looks convoluted. It is.
 */
int
fd_access_rawlock(int fd, int type, int block)
{
	fd_t		*fdp  = fd_array + fd;
	pthread_t	self;

	if (!fdp->lock) {
		SPINUNLOCK(&fdp->spinlock);
		return 1;
	}

	/* Avoid calling into the pthread library too early */
	self = pthread_self();

	/*
	 * If nonblocking, quick test.
	 */
	if (!block) {
		if ((type & FD_WRITE) && fdp->writer && fdp->writer != self)
			goto noblock;
		if ((type & FD_READ)  && fdp->reader && fdp->reader != self)
			goto noblock;
	}

	if (type & FD_WRITE) {
		/*
		 * Look for recursive write lock.
		 */
		if (fdp->writer == self) {
			fdp->writecount++;
			goto rlock;
		}

		/*
		 * Loop on the condition, waiting until the writer is null.
		 */
		while (fdp->writer) {		
			pthread_mutex_lock(fdp->lock);
			fdp->writewait++;
			SPINUNLOCK(&fdp->spinlock);
			pthread_cond_wait(fdp->writecond, fdp->lock);
			pthread_mutex_unlock(fdp->lock);
			SPINLOCK(&fdp->spinlock);
		}

		/*
		 * We got it. Change the state.
		 */
		fdp->writer     = self;
		fdp->writecount = 1;
	}
  rlock:
	if (type & FD_READ) {
		/*
		 * Look for recursive read lock. 
		 */
		if (fdp->reader == self) {
			fdp->readcount++;
			goto done;
		}

		/*
		 * Loop on the condition, waiting until the reader is null.
		 */
		while (fdp->reader) {
			pthread_mutex_lock(fdp->lock);
			fdp->readwait++;
			SPINUNLOCK(&fdp->spinlock);
			pthread_cond_wait(fdp->readcond, fdp->lock);
			pthread_mutex_unlock(fdp->lock);
			SPINLOCK(&fdp->spinlock);
		}

		/*
		 * We got it. Change the state.
		 */
		fdp->reader    = self;
		fdp->readcount = 1;
	}
   done:
	/*
	 * Okay, now the fd spinlock can be safely released.
	 */
	SPINUNLOCK(&fdp->spinlock);
	return 1;
noblock:
	SPINUNLOCK(&fdp->spinlock);
	return 0;
}

void
fd_access_unlock(int fd, int type)
{
	fd_t		*fdp  = fd_array + fd;
	pthread_t	self;

	if (!fdp->lock)
		return;

	/* Avoid calling into the pthread library too early */
	self = pthread_self();

	/*
	 * Use a spinlock to lock the descriptor data structure. 
	 * condition.
	 */
	SPINLOCK(&fdp->spinlock);

	if (type & FD_WRITE) {
		assert(fdp->writer == self);
		
		/*
		 * Look for recursive write lock.
		 */
		if (--fdp->writecount != 0)
			goto rlock;

		/*
		 * Free the writelock and signal any waiters.
		 */
		fdp->writer = 0;

		if (fdp->writewait) {
			pthread_mutex_lock(fdp->lock);
			fdp->writewait = 0;
			SPINUNLOCK(&fdp->spinlock);
			pthread_cond_broadcast(fdp->writecond);
			pthread_mutex_unlock(fdp->lock);
			SPINLOCK(&fdp->spinlock);
		}
	}
  rlock:
	if (type & FD_READ) {
		assert(fdp->reader == self);

		/*
		 * Look for recursive read lock. 
		 */
		if (--fdp->readcount != 0)
			goto done;

		/*
		 * Free the readlock and signal any waiters.
		 */
		fdp->reader = 0;

		if (fdp->readwait) {
			pthread_mutex_lock(fdp->lock);
			fdp->readwait = 0;
			SPINUNLOCK(&fdp->spinlock);
			pthread_cond_broadcast(fdp->readcond);
			pthread_mutex_unlock(fdp->lock);
			return;
		}
	}
   done:
	/*
	 * Okay, now the fd spinlock can be safely released.
	 */
	SPINUNLOCK(&fdp->spinlock);
}
#endif
