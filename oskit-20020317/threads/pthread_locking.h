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

#ifndef _OSKIT_PTHREADS_LOCKING_H_
#define _OSKIT_PTHREADS_LOCKING_H_

#ifdef  THREADS_SPINLOCKS
#define PTHREAD_LOCK_INITIALIZER SPIN_LOCK_INITIALIZER
#define pthread_lock_t spin_lock_t
#define pthread_lock_init(lock) spin_lock_init((spin_lock_t *) lock)
#define pthread_try_lock(lock) spin_try_lock((spin_lock_t *) lock)
#define pthread_lock_locked(lock) spin_lock_locked((spin_lock_t *) lock)
#define pthread_unlock(lock) spin_unlock((spin_lock_t *) lock)

/*
 * When DEBUG is on, try a bit of deadlock detection on locks.
 */
#ifdef	THREADS_DEBUG
extern void pthread_lock_debug(pthread_lock_t *lock);
#define pthread_lock(lock) pthread_lock_debug((spin_lock_t *) lock)
#else
#define pthread_lock(lock) spin_lock((spin_lock_t *) lock)
#endif

#else
#define pthread_lock_t spin_lock_t
#define PTHREAD_LOCK_INITIALIZER	SPIN_LOCK_INITIALIZER
#define pthread_lock_init(lock)		((void)(lock))
#define pthread_try_lock(lock)		((void)(lock), 1)
#define pthread_lock_locked(lock)	((void)(lock), 0)
#define pthread_unlock(lock)		((void)(lock))
#define pthread_lock(lock)		((void)(lock))
#endif

#endif /* _OSKIT_PTHREADS_LOCKING_H_ */
