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

#include "mq.h"

int
mq_unlink(const char *name)
{
#ifdef  THREAD_SAFE
	oskit_error_t	rc;

	rc = oskit_mq_unlink(name);

	if (rc) {
		errno = rc;
		return -1;
	}
	return 0;
#else
	panic("mq_unlink: no single-threaded mq implementation");
	return 0;
#endif
}

#ifdef	THREAD_SAFE

/*
 * Unlink POSIX 1003.4 message queue.
 */
int
oskit_mq_unlink(const char *name)
{
	int i;
	struct mq *mq;

	/* Lock the queue name space */
	pthread_mutex_lock(&mqn_space.mqn_lock);

	/* Unlink the specified queue from the queue name space */
	for (i = 0 ; i < mqn_space.mqn_arraylen ; i++) {
		if (mqn_space.mqn_array[i]
		    && (strcmp(mqn_space.mqn_array[i]->mq_name, name) == 0)) {
			/* Found */
			mq = mqn_space.mqn_array[i];
			/* Lock the message queue descriptor space */
			pthread_mutex_lock(&mqd_space.mqd_lock);
			/* Lock the message queue */
			pthread_mutex_lock(&mq->mq_lock);
			if (mq->mq_refcount != 0) {
				mq->mq_flag |= MQ_UNLINK_FLAG;
				pthread_mutex_unlock(&mq->mq_lock);
			} else {
				mq_queue_remove(mq);
			}
			/*
			 * Unlink the association between the message queue
			 * name space to the message queue.
			 */
			mqn_space.mqn_array[i] = NULL;
			pthread_mutex_unlock(&mqd_space.mqd_lock);
			pthread_mutex_unlock(&mqn_space.mqn_lock);
			return 0;
		}
	}

	/* Unlock the queue name space */
	pthread_mutex_unlock(&mqn_space.mqn_lock);
	return ENOENT;
}

/*
 * Destroy the queue.
 *
 * - When struct mq is actually removed, there is no struct mqdesc
 *   pointing to the struct mq because struct mq::mq_refcount is 0.
 *   Therefore, we have to lock (1) Queue Name Space (to avoid furthur
 *   mq_open() the queue) and (2) Queue Descriptor Table (to avoid
 *   using the queue) while destroying struct mq.
 *
 * - Assume
 *   1) the struct mq is locked (lock is destroyed).
 *   2) the mqn_space.mqn_lock is locked (lock remains),
 *   3) the mqd_space.mqd_lock is locked (lock remains).
 *   4) mq->mq_refcount should be 0.
 */
void
mq_queue_remove(struct mq *mq)
{
	pthread_mutex_unlock(&mq->mq_lock);
	pthread_mutex_destroy(&mq->mq_lock);
	free(mq->mq_name);
	free(mq->mq_chunkblock);
	free(mq);
}
#endif	/* THREAD_SAFE */
