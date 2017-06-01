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
mq_send(mqd_t mqdes, const char *msg_ptr, oskit_size_t msg_len,
	unsigned int msg_prio)
{
#ifdef  THREAD_SAFE
	oskit_error_t	rc;

	rc = oskit_mq_send(mqdes, msg_ptr, msg_len, msg_prio);

	if (rc) {
		errno = rc;
		return -1;
	}
#else
	panic("mq_send: no single-threaded mq implementation");
#endif
	return 0;
}


#ifdef	THREAD_SAFE
int	sigqueue_internal(pid_t pid, int signo,
			  const union sigval value, int code);

#ifdef MQ_SUPPORT_CANCEL
static void send_cancel_handler(struct mq *mq);
#endif

/*
 * Send a message on a POSIX.4 message queue.
 *
 * - This function is cancellation point.
 */
int
oskit_mq_send(mqd_t mqdes, const char *msg_ptr, oskit_size_t msg_len,
	      unsigned int msg_prio)
{
	struct mq *mq;
	struct mq_msg *msg = NULL;
	struct mqdesc *mqdesc;
	int ret = 0;
#ifdef MQ_SUPPORT_CANCEL
	int ocancelstate;

	pthread_testcancel();
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &ocancelstate);
#endif

	/* Check msg_prio */
	if (msg_prio > MQ_PRIO_MAX) {
		ret = EINVAL;
		goto bailout;
	}
	/* Get the queue descriptor from mqdes and lock it */
	mqdesc = mq_desc_lookup_and_lock(mqdes);
	if (mqdesc == NULL) {
		ret = EBADF;
		goto bailout;
	}
	mq = mqdesc->mq;
	pthread_mutex_lock(&mq->mq_lock);	/* lock the queue */
	pthread_mutex_unlock(&mqdesc->lock);	/* unlock the descriptor */

	/* Check open mode */
	if ((mqdesc->oflag & O_ACCMODE) != O_WRONLY
	    && (mqdesc->oflag & O_ACCMODE) != O_RDWR) {
		pthread_mutex_unlock(&mq->mq_lock);
		ret = EACCES;
		goto bailout;
	}

	/* Check msg_len */
	if (msg_len < 0 || msg_len > mq->mq_attr.mq_msgsize) {
		pthread_mutex_unlock(&mq->mq_lock);
		ret = EMSGSIZE;
		goto bailout;
	}

	/* Not to pass already sending threads */
	if (mq->mq_nwriter == 0) {
		/* Get msg block from the free queue */
		msg = mq->mq_free;
	}
	if (msg == NULL) {
		/* No free messsage block is available now */
		if ((mqdesc->oflag & O_NONBLOCK)) {
			pthread_mutex_unlock(&mq->mq_lock);
			ret = EAGAIN;
			goto bailout;
		}
		mq->mq_nwriter++;
#ifdef MQ_SUPPORT_CANCEL
		pthread_cleanup_push((void(*)(void*))send_cancel_handler,
				     (void*)mq);
		pthread_setcancelstate(ocancelstate, NULL);
#endif
		while (mq->mq_free == NULL) {
			/* Wait for there is a room */
			pthread_cond_wait(&mq->mq_notfull, &mq->mq_lock);
		}
#ifdef MQ_SUPPORT_CANCEL
		pthread_cleanup_pop(0);
#endif
		msg = mq->mq_free;
		mq->mq_nwriter--;
	}
	mq->mq_free = msg->msg_next;

	/* Copy the message */
	msg->msg_next = NULL;
	bcopy(msg_ptr, msg->msg, msg_len);
	msg->msg_len = msg_len;

	/* Link to the message queue */
	*(mq->mq_data[msg_prio].mq_tail) = msg;
	mq->mq_data[msg_prio].mq_tail = &msg->msg_next;

	/* Increment curmsgs */
	mq->mq_attr.mq_curmsgs++;

	/* Wakeup receiver if any */
	if (mq->mq_nreader) {
		mq->mq_nreader--;
		pthread_cond_signal(&mq->mq_notempty);
	} else if (mq->mq_attr.mq_curmsgs == 1	   /* sent to empty queue */
		   && (mq->mq_flag & MQ_NOTIFY_FLAG)) {
		/* Notify by a signal if required by mq_notify() */
		sigqueue_internal(0, mq->mq_event.sigev_signo,
				  mq->mq_event.sigev_value, SI_MESGQ);
		/* And, remove the notification registration */
		mq->mq_flag &= ~MQ_NOTIFY_FLAG;
	}
	pthread_mutex_unlock(&mq->mq_lock);
 bailout:
#ifdef MQ_SUPPORT_CANCEL
	pthread_setcancelstate(ocancelstate, NULL);
#endif
	return ret;
}

#ifdef MQ_SUPPORT_CANCEL
static void
send_cancel_handler(struct mq *mq)
{
	mq->mq_nwriter--;
	pthread_mutex_unlock(&mq->mq_lock);
}
#endif

#endif	/* THREAD_SAFE */
