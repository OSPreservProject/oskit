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
mq_close(mqd_t mqdes)
{
#ifdef  THREAD_SAFE
	oskit_error_t	rc;

	rc = oskit_mq_close(mqdes);

	if (rc) {
		errno = rc;
		return -1;
	}
	return 0;
#else
	panic("mq_close: no single-threaded mq implementation");
	return 0;
#endif
}

#ifdef	THREAD_SAFE

/*
 * Terminate access to a POSIX.4 message queue.
 */
int
oskit_mq_close(mqd_t mqdes)
{
	int ret = 0;
	struct mqdesc *mqdesc;
	pthread_mutex_t *lock;

	/* Lock the message queue descriptor space */
	pthread_mutex_lock(&mqd_space.mqd_lock);
	mqdesc = mqd_space.mqd_array[mqdes];
	if (mqdesc) {
		/* Lock the message queue descriptor */
		pthread_mutex_lock(&mqdesc->lock);
		lock = &mqdesc->mq->mq_lock;
		/* Lock the message queue */
		pthread_mutex_lock(lock);
		if (--(mqdesc->mq->mq_refcount) == 0
		    && (mqdesc->mq->mq_flag & MQ_UNLINK_FLAG)) {
			/* Remove the message queue */
			mq_queue_remove(mqdesc->mq);
		} else {
			/* Unlock the message queue */
			pthread_mutex_unlock(lock);
		}
		/* Unlock the message queue descriptor and destroy */
		pthread_mutex_unlock(&mqdesc->lock);
		pthread_mutex_destroy(&mqdesc->lock);
		free(mqdesc);
		mqd_space.mqd_array[mqdes] = NULL;
	} else {
		ret = EBADF;
	}
	/* Unlock the message queue descriptor space */
	pthread_mutex_unlock(&mqd_space.mqd_lock);
	return ret;
}
#endif	/* THREAD_SAFE */
