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

/*
 * Message Queue Descriptor Manipulation
 */

#include "mq.h"

/* Message Queue Descriptor Space */
struct mqdesc_space mqd_space;

/*
 * Allocate Message Queue Descriptor
 */
int
mq_desc_alloc(struct mq *mq, int oflag, mqd_t *mqdesc_ret)
{
	/* extend by `extendby' - which is doubled every time */
	static	int 	extendby = MQ_DESC_EXTENT;
	int		i;
	oskit_size_t 	bytes;
	struct mqdesc	**newarray;

	pthread_mutex_lock(&mqd_space.mqd_lock);

try_again:
	/* search for a free mqdesc */
	for (i = 0 ; i < mqd_space.mqd_arraylen ; i++) {
		if (mqd_space.mqd_array[i] == NULL) {
			/* found an unused slot */
			struct mqdesc *mqdesc;
			/*
			 * Allocate a message queue descriptor object.
			 * free()ed in mq_queue_remove().
			 */
			mqdesc = malloc(sizeof(struct mqdesc));
			if (mqdesc == NULL) {
				return ENOMEM;
			}
			mqdesc->mq = mq;
			mqdesc->oflag = oflag;
			pthread_mutex_init(&mqdesc->lock, NULL);
			mqd_space.mqd_array[i] = mqdesc;
			pthread_mutex_unlock(&mqd_space.mqd_lock);
			*mqdesc_ret = i;
			return 0;
		}
	}

	/* realloc it */
	bytes = (mqd_space.mqd_arraylen + extendby) * sizeof(struct mqdesc *);
	newarray = (struct mqdesc **)realloc(mqd_space.mqd_array, bytes);
	if (newarray == NULL) {
		pthread_mutex_unlock(&mqd_space.mqd_lock);
		return ENOMEM;
	}
	/* zero out new part of the array */
	bzero(newarray + mqd_space.mqd_arraylen,
	      extendby * sizeof(struct mqdesc *));
	mqd_space.mqd_array = newarray;
	mqd_space.mqd_arraylen += extendby;
	extendby *= 2;
	goto try_again;
}

/*
 *  Lookup message queue descriptor from mqd and lock it.
 */
struct mqdesc *
mq_desc_lookup_and_lock(mqd_t mqd)
{
	struct mqdesc *mqdesc;
	pthread_mutex_lock(&mqd_space.mqd_lock);
	mqdesc = mqd_space.mqd_array[mqd];
	if (mqdesc) {
		pthread_mutex_lock(&mqdesc->lock);
	}
	pthread_mutex_unlock(&mqd_space.mqd_lock);
	return mqdesc;
}

#endif	/* THREAD_SAFE */
