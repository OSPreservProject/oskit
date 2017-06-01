/*
 * Copyright (c) 1998 The University of Utah and the Flux Group.
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
 * Maintained in ascending order.
 */

#include <oskit/com/pqueue.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/assert.h>
#include <oskit/c/string.h>

struct pqueue {
	oskit_pqueue_t		p_ioi;
	oskit_u32_t		p_count;

	oskit_size_t		p_size;
	struct pqueue_node	*p_head;
};

struct pqueue_node {
	oskit_iunknown_t	*pn_obj;
	oskit_pqueue_key_t	pn_val;
	struct pqueue_node	*pn_next;
};


static OSKIT_COMDECL
pqueue_query(oskit_pqueue_t *_, const oskit_iid_t *iid, void **out_ihandle)
{
	struct pqueue *pq = (void *)_;

	assert(pq && pq->p_count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_pqueue_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &pq->p_ioi;
                ++pq->p_count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
pqueue_addref(oskit_pqueue_t *_)
{
	struct pqueue *pq = (void *)_;

	assert(pq && pq->p_count);

        return ++pq->p_count;
}

static OSKIT_COMDECL_U
pqueue_release(oskit_pqueue_t *_)
{
	struct pqueue *pq = (void *)_;
	struct pqueue_node *cur, *next;

	assert(pq && pq->p_count);

	if (--pq->p_count)
		return pq->p_count;

	cur = pq->p_head;
	while (cur) {
		next = cur->pn_next;
		oskit_iunknown_release(cur->pn_obj);
		free(cur);
		cur = next;
	}
	free(pq);
	return 0;
}

static OSKIT_COMDECL
pqueue_enqueue(oskit_pqueue_t *_, oskit_iunknown_t *obj, oskit_pqueue_key_t val)
{
	struct pqueue *pq = (void *)_;
	struct pqueue_node *node, *prev, *cur;

        if (pq == NULL || pq->p_count == 0)
                return OSKIT_E_INVALIDARG;

	/*
	 * Make a node we'll glue in somewhere.
	 */
	node = malloc(sizeof *node);
	if (node == NULL)
		return OSKIT_E_OUTOFMEMORY;
	node->pn_obj = obj;
	oskit_iunknown_addref(obj);
	node->pn_val = val;

	/*
	 * Is OBJ gonna be the new head?
	 */
	if (pq->p_head == NULL || val < pq->p_head->pn_val) {
		node->pn_next = pq->p_head;
		pq->p_head = node;
		pq->p_size++;
		return 0;
	}

	/*
	 * OBJ must go somewhere after the head.
	 * Find a guy larger than (or equal to) us and get in front of him.
	 * If we're the largest, get in back.
	 */
	cur = pq->p_head;
	do {
		prev = cur;
		cur = cur->pn_next;
	} while (cur && cur->pn_val < val);
	node->pn_next = cur;
	prev->pn_next = node;
	pq->p_size++;
	return 0;
}

static oskit_iunknown_t * OSKIT_COMCALL
pqueue_front(oskit_pqueue_t *_, oskit_pqueue_key_t *valp)
{
	struct pqueue *pq = (void *)_;

	assert(pq && pq->p_count);

	/* Empty queue? */
	if (pq->p_size == 0)
		return NULL;

	if (valp)
		*valp = pq->p_head->pn_val;
	oskit_iunknown_addref(pq->p_head->pn_obj);
	return pq->p_head->pn_obj;
}

static OSKIT_COMDECL
pqueue_remove(oskit_pqueue_t *_, oskit_iunknown_t *obj)
{
	struct pqueue *pq = (void *)_;
	struct pqueue_node *prev, *cur, *victim;

        if (pq == NULL || pq->p_count == 0)
                return OSKIT_E_INVALIDARG;

	/*
	 * Empty queue?
	 */
	if (pq->p_size == 0)
		return OSKIT_E_FAIL;

	/*
	 * Removing the head?
	 */
	if (pq->p_head->pn_obj == obj) {
		victim = pq->p_head;
		pq->p_head = pq->p_head->pn_next;
		goto found;
	}

	/*
	 * Otherwise removing part of the body.
	 */
	prev = cur = pq->p_head;
	while (cur && cur->pn_obj != obj) {
		prev = cur;
		cur = cur->pn_next;
	}
	if (cur == NULL)
		return OSKIT_E_FAIL;
	victim = cur;
	prev->pn_next = cur->pn_next;

 found:
	oskit_iunknown_release(victim->pn_obj);
	free(victim);
	pq->p_size--;
	return 0;
}

static oskit_size_t OSKIT_COMCALL
pqueue_size(oskit_pqueue_t *_)
{
	struct pqueue *pq = (void *)_;

	assert(pq && pq->p_count);

	return pq->p_size;
}

static oskit_iunknown_t * OSKIT_COMCALL
pqueue_first_satisfying(oskit_pqueue_t *_,
			oskit_bool_t (*predicate)(oskit_iunknown_t *,
						  oskit_pqueue_key_t))
{
	struct pqueue *pq = (void *)_;
	struct pqueue_node *cur;

	assert(pq && pq->p_count);

	if (pq->p_head)
		for (cur = pq->p_head; cur; cur = cur->pn_next)
			if (predicate(cur->pn_obj, cur->pn_val)) {
				oskit_iunknown_addref(cur->pn_obj);
				return cur->pn_obj;
			}
	return NULL;
}


static struct oskit_pqueue_ops pqueue_ops = {
	pqueue_query,
	pqueue_addref,
	pqueue_release,
	pqueue_enqueue,
	pqueue_front,
	pqueue_remove,
	pqueue_size,
	pqueue_first_satisfying,
};


oskit_error_t
oskit_pqueue_create(oskit_pqueue_t **out_pq)
{
	struct pqueue *pq;

	pq = malloc(sizeof *pq);
	if (pq == NULL)
		return OSKIT_E_OUTOFMEMORY;

	pq->p_ioi.ops = &pqueue_ops;
	pq->p_count = 1;
	pq->p_size = 0;
	pq->p_head = NULL;

	*out_pq = &pq->p_ioi;
	return 0;
}

#if 0
#include <oskit/c/stdio.h>
void
oskit_pqueue_dump(oskit_pqueue_t *_)
{
	struct pqueue *pq = (void *)_;
	struct pqueue_node *cur;

	assert(pq && pq->p_count);

	printf("%d: ( ", pq->p_size);
	if (pq->p_head)
		for (cur = pq->p_head; cur; cur = cur->pn_next)
			if (sizeof(oskit_pqueue_key_t) == sizeof(oskit_u64_t))
				printf("<%p,0x%x%08x> ",
				       cur->pn_obj,
				       (oskit_u32_t)(cur->pn_val >> 32),
				       (oskit_u32_t)(cur->pn_val & 0xffffffff));
			else
				printf("<%p,0x%08x> ",
				       cur->pn_obj, cur->pn_val);
	printf(")\n");
}
#endif
