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

#ifndef _OSKIT_C_MQUEUE_H
#define _OSKIT_C_MQUEUE_H

/*
 * External definitions for message queue.
 */

#include <oskit/types.h>
#include <signal.h>

/*
 * Queue object.
 */
typedef int			mqd_t;

#define MQ_PRIO_MAX		16

struct mq_attr {
	long		mq_flags; 	/* Message queue flags */
	long		mq_maxmsg;	/* Maximum number of messages */
	long		mq_msgsize; 	/* Maximum message size */
	long		mq_curmsgs; 	/* # of messages currently queued */
};

/*
 * POSIX API Prototypes
 */
mqd_t		mq_open(const char *name, int oflag, ...);
int		mq_send(mqd_t mqdes, const char *msg_ptr,
			oskit_size_t msg_len, unsigned int msg_prio);
int		mq_receive(mqd_t mqdes, char *msg_ptr,
			   oskit_size_t msg_len, unsigned int *msg_prio);
int		mq_close(mqd_t mqdes);
int		mq_unlink(const char *name);
int		mq_setattr(mqd_t mqdes, const struct mq_attr *mqstat,
			   struct mq_attr *omqstat);
int		mq_getattr(mqd_t mqdes, struct mq_attr *mqstat);
int		mq_notify(mqd_t mqdes, const struct sigevent *notification);

/*
 * OSKIT API Prototypes
 * These functions return error code rather than setting global errno.
 */
int		oskit_mq_open(const char *name, int oflag,
			      mqd_t *mqdes_ret, ...);
int		oskit_mq_send(mqd_t mqdes, const char *msg_ptr,
			      oskit_size_t msg_len, unsigned int msg_prio);
int		oskit_mq_receive(mqd_t mqdes, char *msg_ptr,
				 oskit_size_t msg_len, unsigned int *msg_prio,
				 int *nreceived);
int		oskit_mq_close(mqd_t mqdes);
int		oskit_mq_unlink(const char *name);
int		oskit_mq_setattr(mqd_t mqdes, const struct mq_attr *mqstat,
				 struct mq_attr *omqstat);
int		oskit_mq_getattr(mqd_t mqdes, struct mq_attr *mqstat);
int		oskit_mq_notify(mqd_t mqdes, 
				const struct sigevent *notification);

#endif /* _OSKIT_C_MQUEUE_H */
