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
mq_receive(mqd_t mqdes, char *msg_ptr,
	   oskit_size_t msg_len, unsigned int *msg_prio)
{
#ifdef  THREAD_SAFE
	oskit_error_t	rc;
	int nreceived;

	rc = oskit_mq_receive(mqdes, msg_ptr, msg_len, msg_prio, &nreceived);

	if (rc) {
		errno = rc;
		return -1;
	}
	return nreceived;
#else
	panic("mq_receive: no single-threaded mq implementation");
	return 0;
#endif
}


#ifdef	THREAD_SAFE

#ifdef MQ_SUPPORT_CANCEL
static void receive_cancel_handler(struct mq *mq);
#endif

/*
 * Receive a message from a POSIX.4 message queue.
 */
int
oskit_mq_receive(mqd_t mqdes, char *msg_ptr,
		 oskit_size_t msg_len, unsigned int *msg_prio,
		 int *nreceived)
{
	struct mqdesc *mqdesc;
	struct mq *mq;
	struct mq_msg *msg = NULL;
	int prio = 0;
	int ret = 0;
#ifdef MQ_SUPPORT_CANCEL
	int ocancelstate;

	pthread_testcancel();
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &ocancelstate);
#endif

	/* Get the queue from mqdes */
	mqdesc = mq_desc_lookup_and_lock(mqdes);
	if (mqdesc == NULL) {
		ret = EBADF;
		goto bailout;
	}
	/* Check open mode */
	if ((mqdesc->oflag & O_ACCMODE) != O_RDONLY
	    && (mqdesc->oflag & O_ACCMODE) != O_RDWR) {
		pthread_mutex_unlock(&mqdesc->lock);
		ret = EACCES;
		goto bailout;
	}

	mq = mqdesc->mq;
	pthread_mutex_lock(&mq->mq_lock);	/* lock the queue */
	pthread_mutex_unlock(&mqdesc->lock);	/* unlock the descriptor */

	/* Check msg_len */
	if (msg_len < mq->mq_attr.mq_msgsize) {
		pthread_mutex_unlock(&mq->mq_lock);
		ret = EMSGSIZE;
		goto bailout;
	}

	/* Not to pass already waiting threads */
	if (mq->mq_nreader == 0 && mq->mq_attr.mq_curmsgs != 0) {
 retry:
		/* Search and unlink a message from the queue.  XXX bitmap? */
		for (prio = MQ_PRIO_MAX ; prio >= 0 ; prio--) {
			if (mq->mq_data[prio].mq_head) {
				msg = mq->mq_data[prio].mq_head;
				mq->mq_data[prio].mq_head = msg->msg_next;
				if (mq->mq_data[prio].mq_tail
				    == &msg->msg_next) {
					mq->mq_data[prio].mq_tail
						= &mq->mq_data[prio].mq_head;
				}
				break;
			}
		}
	}

	if (msg == NULL) {
		/* No messsage block is stored now */
		if ((mqdesc->oflag & O_NONBLOCK)) {
			pthread_mutex_unlock(&mq->mq_lock);
			ret = EAGAIN;
			goto bailout;
		}
		mq->mq_nreader++;
#ifdef MQ_SUPPORT_CANCEL
		pthread_cleanup_push((void(*)(void*))receive_cancel_handler,
				     (void*)mq);
		pthread_setcancelstate(ocancelstate, NULL);
#endif
		/* Wait for there is a room */
		pthread_cond_wait(&mq->mq_notempty, &mq->mq_lock);
#ifdef MQ_SUPPORT_CANCEL
		pthread_cleanup_pop(0);
#endif
		/* Note that `mq->mq_nreader--' is done by the sender side */
		goto retry;
	}

	/* Copy the message */
	bcopy(msg->msg, msg_ptr, msg->msg_len);

	if (msg_prio) {
		*msg_prio = prio;
	}

	/* Return the message block to the free queue */
	msg->msg_next = mq->mq_free;
	mq->mq_free = msg;

	/* Decrement curmsgs */
	mq->mq_attr.mq_curmsgs--;

	/* Wakeup sender if any */
	if (mq->mq_nwriter) {
		pthread_cond_signal(&mq->mq_notfull);
	}

	pthread_mutex_unlock(&mq->mq_lock);
	*nreceived = msg->msg_len;

 bailout:
#ifdef MQ_SUPPORT_CANCEL
	pthread_setcancelstate(ocancelstate, NULL);
#endif
	return ret;
}

#ifdef MQ_SUPPORT_CANCEL
static void
receive_cancel_handler(struct mq *mq)
{
	mq->mq_nreader--;
	pthread_mutex_unlock(&mq->mq_lock);
}
#endif

#endif	/* THREAD_SAFE */
