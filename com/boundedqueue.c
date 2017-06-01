/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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

/*
 * a bounded queue implementation of the oskit_queue interface
 */
#include <oskit/com/queue.h>
#include <oskit/com/listener.h>
#include <oskit/c/assert.h>
#include <oskit/c/malloc.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>
#include <oskit/c/assert.h>

typedef struct queue {
	oskit_queue_t	ioi;
	unsigned	count;
	void		*array;
	int		head, tail;
	int		itemsize;
	int		qlen, maxqlen;
	int		droplast;
	oskit_listener_t *l;
} queue_t;

static OSKIT_COMDECL
queue_query(oskit_queue_t *s, const oskit_iid_t *iid, void **out_ihandle)
{
	queue_t *q = (void *)s;

	assert(q && q->count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_queue_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &q->ioi;
                ++q->count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
queue_addref(oskit_queue_t *s)
{
	queue_t *q = (void *)s;

        assert(q->count);

        return ++q->count;
}

static OSKIT_COMDECL_U
queue_release(oskit_queue_t *s)
{
	queue_t *q = (void *)s;
        unsigned newcount;

	assert(q && q->count);

        newcount = q->count - 1;

        if (newcount == 0) {
		if (q->l)
			oskit_listener_release(q->l);

		free(q->array);
                free(q);
                return 0;
        }

        return q->count = newcount;
}

static OSKIT_COMDECL_U
queue_dequeue(oskit_queue_t *s, void *item)
{
	queue_t *q = (void *)s;

	/* make sure queue is not empty... */
	assert(q->head != q->tail);

	memcpy(item, q->array + q->tail * q->itemsize, q->itemsize);
	q->tail = (q->tail + 1) % (q->maxqlen + 1);
	q->qlen--;

	return q->itemsize;
}

static OSKIT_COMDECL
queue_enqueue(oskit_queue_t *s, const void *item, oskit_size_t size)
{
	queue_t *q = (void *)s;
	oskit_error_t	rc = 0;

	/* Is this queue is full -> tell listener
	 * that I'm about to dump an entry
	 */
	if (q->qlen == q->maxqlen) {
		if (q->l)
			oskit_listener_notify(q->l, (oskit_iunknown_t *)s);

		/* if nothing has happened, then dump it, for christ's sake */
		if (q->qlen == q->maxqlen) {
			if (q->droplast) {
				q->tail = (q->tail + 1) % (q->maxqlen + 1);
				q->qlen--;
			} else
				return OSKIT_ENOMEM;
		}
	}

	memcpy(q->array + q->head * q->itemsize, item,
		q->itemsize < size ? q->itemsize : size);
	q->head = (q->head + 1) % (q->maxqlen + 1);
	q->qlen++;
	return rc;
}

static OSKIT_COMDECL_U
queue_size(oskit_queue_t *s)
{
	return ((queue_t *)s)->qlen;
}

static OSKIT_COMDECL_U
queue_front(oskit_queue_t *s, void *item)
{
	return OSKIT_E_NOTIMPL;
}

static struct oskit_queue_ops oskit_queue_ops = {
	queue_query,
	queue_addref,
	queue_release,
	queue_enqueue,
	queue_dequeue,
	queue_size,
	queue_front
};

oskit_queue_t *
create_bounded_queue_with_fixed_size_items(int qlen,
		oskit_size_t 	itemsize,
		oskit_listener_t *l,
		int	droplast)
{
	queue_t *q = malloc(sizeof *q);
	if (!q)
		return NULL;

	memset (q, 0, sizeof *q);
	q->maxqlen = qlen;
	q->itemsize = itemsize;
	if ((q->l = l) != NULL)
		oskit_listener_addref(l);

	q->array = calloc(qlen + 1, itemsize);

	q->ioi.ops = &oskit_queue_ops;
	q->count = 1;
	q->droplast = droplast;

	return &q->ioi;
}
