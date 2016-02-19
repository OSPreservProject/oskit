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

#ifndef _THREADS_MQUEUE_MQ_H_
#define _THREADS_MQUEUE_MQ_H_

#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <oskit/c/fcntl.h>
#include <oskit/threads/pthread.h>
#include <oskit/c/mqueue.h>

/*
 * Queue Name Space      array of ptr 
 * (mqn_space)           to struct mq
 * +---------------+     +-------+
 * |               |---->|       |-----------------------,                    
 * |     Lock      |     |-------|                       |                    
 * +---------------+     |       |                       |    struct mq       
 *                       |-------|                       |   +---------------+
 *                                                       +-->|               |
 *                                                       |   |     Lock      |
 * Queue Descriptor                                      |   +---------------+
 * Table               array of ptr     struct mqdesc    |                    
 * (mqd_space)         to struct mqdesc                  |
 * +---------------+     +-------+     +---------------+ |   
 * |               |---->|   0   |---->|               |-'
 * |     Lock      |     |-------|     |     Lock      |
 * +---------------+     |   1   |     +---------------+
 *                       |-------|
 *                       |   2.. |
 *                       |-------|
 * 
 *
 * - To avoid stupid dead locks, be sure to lock in this order.
 *
 *	1. mqn_space.mqn_lock
 *	2. mqd_space.mqd_lock
 *	3. struct mqdesc
 *	4. struct mq
 *
 */

/* Supoort thread cancellation in mq_send() and mq_receive() */
#define MQ_SUPPORT_CANCEL

/* # of message queue descriptor allocated at first */
#define MQ_DESC_INITIAL_SIZE	20
#define MQ_DESC_EXTENT        	10

/* # of message queue space allocated at first */
#define MQ_INITIAL_SIZE		20


/*
 * Internal message queue structure.
 */


/*
 * Header for a message in a message queue.  Per one message.
 */
struct mq_msg {
	struct mq_msg		*msg_next;
	oskit_size_t		msg_len;	/* data length */
	char			msg[0];		/* data */
};

/*
 * Internal Message Queue.  Per one queue.
 * Do not realloc() this structure.
 */
struct mq {
	char			*mq_name;	/* queue name */
	oskit_mode_t		mq_mode; 	/* queue permission */
	struct mq_attr		mq_attr;	/* attribute */

	int			mq_refcount;	/* reference count */
	int			mq_flag;
#define MQ_UNLINK_FLAG	1
#define MQ_NOTIFY_FLAG	2
	struct mq_msg		*mq_chunkblock;	/* malloc'ed chunk block */

	struct {
		struct mq_msg	*mq_head;
		struct mq_msg	**mq_tail;
	}			mq_data[MQ_PRIO_MAX+1];
	struct mq_msg		*mq_free;	/* free chunks */

	/* synchronize stuff */
	pthread_mutex_t		mq_lock;
	pthread_cond_t		mq_notempty;
	pthread_cond_t		mq_notfull;
	int			mq_nwriter;	/* # of waiting writer */
	int			mq_nreader;	/* # of waiting reader */

	/* for mq_notify() */
	struct sigevent		mq_event;
};

/*
 * Queue Namespace.  Per one system.
 */
struct mqn_space {
	struct mq		**mqn_array;
	int			mqn_arraylen;
	pthread_mutex_t		mqn_lock;
};
extern struct mqn_space	mqn_space;

/*
 * Structure representing a message queue descriptor table entry.
 */
struct mqdesc {
	struct mq		*mq;
	oskit_mode_t		oflag; /* open mode from mq_open()*/
	pthread_mutex_t		lock;
};

/*
 * Message Queue Descriptor Space.  Per one system.
 */
struct mqdesc_space {
	struct mqdesc		**mqd_array;
	int			mqd_arraylen;
	pthread_mutex_t		mqd_lock;
};
extern struct mqdesc_space mqd_space;

/*
 * Prototypes
 */
mqd_t		mq_desc_alloc(struct mq *mq, int oflag, mqd_t *mqdesc_ret);
struct mqdesc	*mq_desc_lookup_and_lock(mqd_t mqd);
void		mq_queue_remove(struct mq *mq);
void		mq_start(void);

#endif /* _THREADS_MQUEUE_MQ_H_ */
