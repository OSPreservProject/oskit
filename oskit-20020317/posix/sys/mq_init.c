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

#ifdef	THREAD_SAFE
#include "mq.h"

static void mq_cleanup(void);

/*
 * Initialize the POSIX message queue
 */
void
mq_start(void)
{
	/* Initialize Message Queue Descriptor Space */
	pthread_mutex_init(&mqd_space.mqd_lock, NULL);

	mqd_space.mqd_array =
		(struct mqdesc **)calloc(MQ_DESC_INITIAL_SIZE,
					 sizeof(struct mqdesc *));
	if (mqd_space.mqd_array == NULL) {
		panic("mq_start: cannot allocate memory");
	}
	mqd_space.mqd_arraylen = MQ_DESC_INITIAL_SIZE;

	/* Initialize Message Queue Name Space */
	pthread_mutex_init(&mqn_space.mqn_lock, NULL);

	mqn_space.mqn_array = calloc(MQ_INITIAL_SIZE, sizeof(struct mq *));
	if (mqn_space.mqn_array == NULL) {
		panic("mq_start: cannot allocate memory");
	}
	mqn_space.mqn_arraylen = MQ_INITIAL_SIZE;

	/* Register cleanup function */
	atexit(mq_cleanup);
}

/*
 * Cleanup routine.
 */
static void
mq_cleanup(void)
{
	int	i;

#if 0
	printf("MQ_CLEANUP\n");
#endif
	for (i = 0; i < mqd_space.mqd_arraylen; i++) {
		if (mqd_space.mqd_array[i]) {
			oskit_mq_close(i);
		}
	}
	pthread_mutex_lock(&mqn_space.mqn_lock);
	pthread_mutex_lock(&mqd_space.mqd_lock);
	for (i = 0; i < mqn_space.mqn_arraylen; i++) {
		if (mqn_space.mqn_array[i]) {
			pthread_mutex_lock(&mqn_space.mqn_array[i]->mq_lock);
			mq_queue_remove(mqn_space.mqn_array[i]);
		}
	}
	pthread_mutex_unlock(&mqd_space.mqd_lock);
	pthread_mutex_unlock(&mqn_space.mqn_lock);
	pthread_mutex_destroy(&mqd_space.mqd_lock);
	pthread_mutex_destroy(&mqn_space.mqn_lock);
	free(mqd_space.mqd_array);
	free(mqn_space.mqn_array);
}
#endif	/* THREAD_SAFE */
