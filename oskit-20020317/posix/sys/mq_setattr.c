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
mq_setattr(mqd_t mqdes, const struct mq_attr *mqstat, struct mq_attr *omqstat)
{
#ifdef  THREAD_SAFE
	oskit_error_t	rc;

	rc = oskit_mq_setattr(mqdes, mqstat, omqstat);

	if (rc) {
		errno = rc;
		return -1;
	}
	return 0;
#else
	panic("mq_setattr: no single-threaded mq implementation");
	return 0;
#endif
}


#ifdef	THREAD_SAFE

/*
 * Set a subset of POSIX.4 message queue attributes.
 */
int
oskit_mq_setattr(mqd_t mqdes, const struct mq_attr *mqstat,
		 struct mq_attr *omqstat)
{
	struct mq *mq;
	struct mqdesc *mqdesc;

	/* Get the queue descriptor from mqdes and lock it */
	mqdesc = mq_desc_lookup_and_lock(mqdes);
	if (mqdesc == NULL) {
		return EBADF;
	}
	mq = mqdesc->mq;
	pthread_mutex_lock(&mq->mq_lock);	/* lock the queue */
	pthread_mutex_unlock(&mqdesc->lock);	/* unlock the descriptor */

	if (omqstat) {
		*omqstat = mq->mq_attr;
	}
	mqdesc->oflag = ((mqdesc->oflag & ~O_NONBLOCK)
			 | (mqstat->mq_flags & O_NONBLOCK));

	pthread_mutex_unlock(&mq->mq_lock);
	return 0;
}

#endif	/* THREAD_SAFE */
