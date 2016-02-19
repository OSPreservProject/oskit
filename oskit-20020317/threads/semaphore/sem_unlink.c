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
 * Remove a Named Semaphore
 */
int
oskit_sem_unlink(const char *name)
{
	int i;
	sem_t *sem;

	/* Lock the semaphore name space */
	pthread_mutex_lock(&semn_space.semn_lock);

	/* Unlink the specified queue from the queue name space */
	for (i = 0 ; i < semn_space.semn_arraylen ; i++) {
		if (semn_space.semn_array[i]
		    && (strcmp(semn_space.semn_array[i]->sem_name, name)
			== 0)) {
			/* Found */
			sem = semn_space.semn_array[i];
			/* Lock the semaphore */
			pthread_lock(&sem->sem_lock);
			if (sem->sem_refcount != 0) {
				sem->sem_flag |= SEM_UNLINK_FLAG;
				pthread_unlock(&sem->sem_lock);
			} else {
				sem_remove(sem);
			}
			/*
			 * Unlink the association between the semaphore
			 * name space to the semaphore.
			 */
			semn_space.semn_array[i] = NULL;
			pthread_mutex_unlock(&semn_space.semn_lock);
			return 0;
		}
	}

	/* Unlock the queue name space */
	pthread_mutex_unlock(&semn_space.semn_lock);
	return ENOENT;
}

/*
 * Destroy the queue.
 *
 * - When struct mq is actually removed, there is no struct mqdesc
 *   pointing to the struct mq because struct mq::sem_refcount is 0.
 *   Therefore, we have to lock (1) Queue Name Space (to avoid furthur
 *   sem_open() the queue) and (2) Queue Descriptor Table (to avoid
 *   using the queue) while destroying struct mq.
 *
 * - Assume
 *   1) the sem_t is locked
 *   2) the semn_space.semn_lock is locked (lock remains),
 *   4) sem->refcount should be 0.
 */
void
sem_remove(sem_t *sem)
{
	if (sem->sem_name) {
		free(sem->sem_name);
	}
	free(sem);
}
