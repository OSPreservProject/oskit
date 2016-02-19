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

static void sem_cleanup(void);

/*
 * Initialize the message queue descriptor name space.
 */
void
sem_start(void)
{
	/* Initialize Message Queue Name Space */
	pthread_mutex_init(&semn_space.semn_lock, NULL);

	semn_space.semn_array = calloc(SEM_INITIAL_SIZE, sizeof(sem_t *));
	if (semn_space.semn_array == NULL) {
		panic("sem_start: cannot allocate memory");
	}
	semn_space.semn_arraylen = SEM_INITIAL_SIZE;

	/* Register cleanup function */
	atexit(sem_cleanup);
}

/*
 * Cleanup routine.
 */
static void
sem_cleanup(void)
{
	/* leave semaphores not released */
#if 0
	int		i;

#ifdef DEBUG
	printf("SEM_CLEANUP\n");
#endif
	pthread_mutex_lock(&semn_space.semn_lock);
	for (i = 0; i < semn_space.semn_arraylen; i++) {
		if (semn_space.semn_array[i]) {
			pthread_mutex_lock(&semn_space.semn_array[i]->sem_lock);
			sem_remove(semn_space.semn_array[i]);
		}
	}
	pthread_mutex_unlock(&semn_space.semn_lock);
	pthread_mutex_destroy(&semn_space.semn_lock);
	free(semn_space.semn_array);
#endif
}
