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
mq_notify(mqd_t mqdes, const struct sigevent *notification)
{
#ifdef  THREAD_SAFE
	oskit_error_t	rc;

	rc = oskit_mq_notify(mqdes, notification);

	if (rc) {
		errno = rc;
		return -1;
	}
#else
	panic("mq_notify: no single-threaded mq implementation");
#endif
	return 0;
}


#ifdef	THREAD_SAFE

/*
 * Register a request to be notified when a message arrives on an empty
 * message queue.
 *
 * POSIX says as follows
 *
 * - a signal is sent when a message is sent to empty queue.
 * - only one process may be registered for notification by a message queue.
 * - when the notification is sent to the registered process, its registration
 *   shall be removed.
 */
int
oskit_mq_notify(mqd_t mqdes, const struct sigevent *notification)
{
	struct mq *mq;
	struct mqdesc *mqdesc;
	int ret = 0;

	/* Get the queue descriptor from mqdes and lock it */
	mqdesc = mq_desc_lookup_and_lock(mqdes);
	if (mqdesc == NULL) {
		return EBADF;
	}
	mq = mqdesc->mq;
	pthread_mutex_lock(&mq->mq_lock);	/* lock the queue */
	pthread_mutex_unlock(&mqdesc->lock);	/* unlock the descriptor */

	if (notification != NULL) {
		if (notification->sigev_signo < 1
		    || notification->sigev_signo >= NSIG) {
			ret = EINVAL;
		}
		switch (notification->sigev_notify) {
		case SIGEV_NONE:
			/* XXX: I'm not sure this code is right or not. */
			mq->mq_flag &= ~MQ_NOTIFY_FLAG;
			break;
		case SIGEV_SIGNAL:
			mq->mq_flag |= MQ_NOTIFY_FLAG;
			mq->mq_event = *notification;
			break;
		default:
			ret = EINVAL;
			break;
		}
	} else {
		mq->mq_flag &= ~MQ_NOTIFY_FLAG;
	}

	pthread_mutex_unlock(&mq->mq_lock);
	return ret;
}
#endif	/* THREAD_SAFE */
