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

mqd_t
mq_open(const char *name, int oflag, ...)
{
#ifdef  THREAD_SAFE
	oskit_error_t	rc;
	mqd_t mqdes;
	oskit_mode_t mode;
	const struct mq_attr *attr;
	va_list pvar;

	va_start(pvar, oflag);
	mode = va_arg(pvar, oskit_mode_t);
	attr = va_arg(pvar, struct mq_attr*);
	va_end(pvar);

	rc = oskit_mq_open(name, oflag, &mqdes, mode, attr);

	if (rc) {
		errno = rc;
		return -1;
	}
	return mqdes;
#else
	panic("mq_open: no single-threaded mq implementation");
	return 0;
#endif
}

#ifdef	THREAD_SAFE
/* Queue Name Space */
struct mqn_space	mqn_space;

static struct mq *mq_namespace_lookup(const char *name);
static int mq_create_queue(const char *name, oskit_mode_t mode,
			   int oflag, const struct mq_attr *attr,
			   struct mq **mq_ret);
static int mq_namespace_register(struct mq *mq);

/* default attribute for mq_open() used when attr == NULL */
static const struct mq_attr attr_default = {
	0,	/* mq_flags */
	1,	/* mq_maxmsg */
	4096,	/* mq_msgsize */
	0	/* mq_curmsgs */
};

/*
 * Create/access a POSIX.4 message queue.
 */
int
oskit_mq_open(const char *name, int oflag, mqd_t *mqdes, ...)
{
	struct mq *mq;
	int ret = 0;
	oskit_mode_t mode;
	const struct mq_attr *attr;
	int created = 0;
	va_list pvar;

	va_start(pvar, mqdes);
	mode = va_arg(pvar, oskit_mode_t);
	attr = va_arg(pvar, struct mq_attr*);
	va_end(pvar);

	/* check oflag arg */
	switch ((oflag & O_ACCMODE)) {
	case O_RDONLY:
	case O_WRONLY:
	case O_RDWR:
		break;
	default:
		return EACCES;
	}

	/* lock the queue name space */
	pthread_mutex_lock(&mqn_space.mqn_lock);

	mq = mq_namespace_lookup(name);
	if (mq == NULL) {
		if (!(oflag & O_CREAT)) {
			pthread_mutex_unlock(&mqn_space.mqn_lock);
			return ENOENT;
		}
		if (attr == NULL) {
			/*
			 * If attr is NULL, the message queue is created with
			 * implemetation-defined default message queue
			 * attributes (POSIX).
			 */
			attr = &attr_default;
		}

		ret = mq_create_queue(name, mode, oflag, attr, &mq);
		if (ret != 0) {
			pthread_mutex_unlock(&mqn_space.mqn_lock);
			return ret;
		}
		created = 1;
	} else {
		if ((oflag & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL)) {
			pthread_mutex_unlock(&mqn_space.mqn_lock);
			return EEXIST;
		}
		mq->mq_refcount++;
	}

	/* assign a message queue descriptor */
        ret = mq_desc_alloc(mq, oflag, mqdes);
	if (ret != 0) {
		/*
		 * If we've just created a message queue, it must be removed.
		 */
		if (created) {
			pthread_mutex_lock(&mq->mq_lock);
			mq->mq_refcount = 0;
			mq_queue_remove(mq);
		}
	}
	pthread_mutex_unlock(&mqn_space.mqn_lock);

        return ret;
}

static struct mq *
mq_namespace_lookup(const char *name)
{
	int i;

	for (i = 0 ; i < mqn_space.mqn_arraylen ; i++) {
		if (mqn_space.mqn_array[i]
		    && (strcmp(mqn_space.mqn_array[i]->mq_name, name) == 0)) {
			return mqn_space.mqn_array[i];
		}
	}
	return NULL;
}

/*
 * Create and register a message queue
 */
static int
mq_create_queue(const char *name, oskit_mode_t mode, int oflag, 
		const struct mq_attr *attr, struct mq **mq_ret)
{
	struct mq *mq;
	int chunksz;
	int ret;
	int i;
	struct mq_msg *chunkblock = NULL;

	/* allocate memory for the queue object */
	mq = malloc(sizeof(struct mq));
	if (mq == NULL) {
		ret = ENOMEM;
		goto errorexit;
	}
	mq->mq_name = NULL;

	chunksz = (offsetof(struct mq_msg, msg[0]) + attr->mq_msgsize);

	/* allocate message buffer for the queue */
	chunkblock = malloc(chunksz * attr->mq_maxmsg);
	if (chunkblock == NULL) {
		ret = ENOMEM;
		goto errorexit;
	}
		
	/* set each field of mq */
	mq->mq_name = strdup(name);
	if (mq->mq_name == NULL) {
		ret = ENOMEM;
		goto errorexit;
	}
	mq->mq_mode = mode;
	mq->mq_attr = *attr;
	mq->mq_attr.mq_curmsgs = 0;
	
	mq->mq_refcount = 1;
	mq->mq_flag = 0;
	mq->mq_chunkblock = chunkblock;
	for (i = 0 ; i <= MQ_PRIO_MAX ; i++) {
		mq->mq_data[i].mq_head = NULL;
		mq->mq_data[i].mq_tail = &mq->mq_data[i].mq_head;
	}
	pthread_mutex_init(&mq->mq_lock, NULL);
	pthread_cond_init(&mq->mq_notempty, NULL);
	pthread_cond_init(&mq->mq_notfull, NULL);
	mq->mq_nwriter = 0;
	mq->mq_nreader = 0;

	/* split the message buffer and link it to the free queue */
	mq->mq_free = NULL;
	for (i = 0 ; i < attr->mq_maxmsg ; i++) {
		struct mq_msg *chunk;

		chunk = (struct mq_msg *)((void *)chunkblock + chunksz * i);
		chunk->msg_len = 0;
		chunk->msg_next = mq->mq_free;
		mq->mq_free = chunk;
	}
	/* register to the queue name space */
	if (mq_namespace_register(mq) < 0) {
		ret = ENOMEM;
		goto errorexit;
	}

	*mq_ret = mq;
	return 0;

 errorexit:
	if (mq) {
		if (mq->mq_name) {
			free(mq->mq_name);
		}
		free(mq);
	}
	if (chunkblock) {
		free(chunkblock);
	}
	return ret;
}

/* Assume that mqn_space.mqn_lock is locked */
static int
mq_namespace_register(struct mq *mq)
{
	int i;

	/* Register to the queue name space.  XXX should be hash */
	for (i = 0 ; i < mqn_space.mqn_arraylen ; i++) {
		if (mqn_space.mqn_array[i] == NULL) {
			mqn_space.mqn_array[i] = mq;
			break;
		}
	}

	if (i == mqn_space.mqn_arraylen) {
		struct mq **tmp;

		/* realloc the queue name space */
		tmp = realloc(mqn_space.mqn_array, 
			      (mqn_space.mqn_arraylen * 2)
			      * sizeof(struct mq*));
		if (tmp == NULL) {
			return -1;
		}
		bzero(tmp + mqn_space.mqn_arraylen, 
		      mqn_space.mqn_arraylen * sizeof(struct mq*));
		tmp[mqn_space.mqn_arraylen] = mq;
		mqn_space.mqn_arraylen *= 2;
		mqn_space.mqn_array = tmp;
	}
	return 0;
}
#endif	/* THREAD_SAFE */
