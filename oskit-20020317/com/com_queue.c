/*
 * Copyright (c) 1998, 1999 The University of Utah and the Flux Group.
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
 * A queue of pointers to COM objects.
 * Reference counts are handled via oskit_iunknown_addref, etc.
 *
 * NOTE: this doesn't use the notion of size as specified in the queue
 * interface.
 * This is because it isn't needed for allocating and managing the queue.
 * This means that `enqueue' ignores the `size' argument and `dequeue',
 * and `front' do not return it.
 */

#include <oskit/com/queue.h>
#include <oskit/c/assert.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>

struct queue {
	oskit_queue_t	q_ioi;
	oskit_u32_t	q_count;

	oskit_size_t	q_curlength;
	oskit_size_t	q_maxlength;
	oskit_size_t	q_head;
	oskit_size_t	q_tail;
	oskit_iunknown_t *q_array[1];
};

#define increment(q,c)	(q->c = (q->c + 1) % q->q_maxlength)


static OSKIT_COMDECL
queue_query(oskit_queue_t *_, const oskit_iid_t *iid, void **out_ihandle)
{
	struct queue *q = (void *)_;

	assert(q && q->q_count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_queue_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &q->q_ioi;
                ++q->q_count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
queue_addref(oskit_queue_t *_)
{
	struct queue *q = (void *)_;

	assert(q && q->q_count);

        if (q == NULL || q->q_count == 0)
                return OSKIT_E_INVALIDARG;

        return ++q->q_count;
}

static OSKIT_COMDECL_U
queue_release(oskit_queue_t *_)
{
	struct queue *q = (void *)_;
	oskit_size_t i;

	assert(q && q->q_count);

	if (--q->q_count)
		return q->q_count;

	for (i = 0; i < q->q_maxlength; i++)
		if (q->q_array[i])
			oskit_iunknown_release(q->q_array[i]);
	free(q);
	return 0;
}

static OSKIT_COMDECL
queue_enqueue(oskit_queue_t *_, const void *item, oskit_size_t ignored_size)
{
	struct queue *q = (void *)_;
	oskit_iunknown_t *obj = (void *)item;

	assert(q && q->q_count);

	/* If the queue is full, drop it. */
	if (q->q_array[q->q_tail] != NULL) {
		return OSKIT_E_FAIL;
	}

	/* Otherwise we have space. */
	q->q_array[q->q_tail] = obj;
	oskit_iunknown_addref(obj);
	increment(q,q_tail);
	q->q_curlength++;
	return 0;
}

static OSKIT_COMDECL_U
queue_dequeue(oskit_queue_t *_, void *item)
{
	struct queue *q = (void *)_;
	oskit_iunknown_t **obj = item;

	assert(q && q->q_count);

	/* Nothing to dequeue? */
	assert(q->q_curlength != 0);
	/* Head shouldn't be NULL if q_curlength is non-zero. */
	assert(q->q_array[q->q_head] != NULL);

	/* Otherwise, take the head elem. */
	*obj = q->q_array[q->q_head];		/* they get our ref */
	q->q_array[q->q_head] = NULL;
	increment(q,q_head);
	q->q_curlength--;
	return 0;
}

static OSKIT_COMDECL_U
queue_size(oskit_queue_t *_)
{
	struct queue *q = (void *)_;

	assert(q && q->q_count);

	return q->q_curlength;
}

static OSKIT_COMDECL_U
queue_front(oskit_queue_t *_, void *item)
{
	struct queue *q = (void *)_;
	oskit_iunknown_t **obj = item;

	assert(q && q->q_count);

	/* Empty queue? */
	assert(q->q_curlength != 0);
	/* Head shouldn't be NULL if q_curlength is non-zero. */
	assert(q->q_array[q->q_head] != NULL);

	/* Give them a pointer to the head. */
	*obj = q->q_array[q->q_head];
	oskit_iunknown_addref(*obj);
	return 0;
}

static struct oskit_queue_ops queue_ops = {
	queue_query,
	queue_addref,
	queue_release,
	queue_enqueue,
	queue_dequeue,
	queue_size,
	queue_front
};


oskit_error_t
oskit_bounded_com_queue_create(oskit_size_t length, oskit_queue_t **out_q)
{
	struct queue *q;

	if (length == 0)
		return OSKIT_E_INVALIDARG;

	q = calloc(sizeof(struct queue) + (length-1)*sizeof(oskit_iunknown_t), 1);
	if (q == NULL)
		return OSKIT_E_OUTOFMEMORY;

	q->q_ioi.ops = &queue_ops;
	q->q_count = 1;

	q->q_curlength = 0;
	q->q_maxlength = length;
	q->q_head = 0;
	q->q_tail = 0;
	/* q_array is already zeroed */

	*out_q = &q->q_ioi;
	return 0;
}

#if 0
#include <oskit/c/stdio.h>
void
oskit_bounded_com_queue_dump(oskit_queue_t *_)
{
	struct queue *q = (void *)_;
	oskit_size_t i;

	assert(q && q->q_count);

	printf("%d (%d,%d): [ ", q->q_curlength, q->q_head, q->q_tail);
	for (i = 0; i < q->q_maxlength; i++)
		printf("%p ", q->q_array[i]);
	printf("]\n");
}
#endif
