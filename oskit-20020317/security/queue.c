/* FLASK */

/*
 * Copyright (c) 1999, 2000 The University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * Contributed by the Computer Security Research division,
 * INFOSEC Research and Technology Office, NSA.
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

#ifndef __KERNEL__

/*
 * Implementation of the double-ended queue type.
 */

#include "queue.h"

queue_t
queue_create(void)
{
	queue_t q;

	q = (queue_t) malloc(sizeof(struct queue_info));
	if (q == NULL)
		return NULL;

	q->head = q->tail = NULL;

	return q;
}

int queue_insert(queue_t q, queue_element_t e)
{
	queue_node_ptr_t newnode;


	if (!q)
		return -1;

	newnode = (queue_node_ptr_t) malloc(sizeof(struct queue_node));
	if (newnode == NULL)
		return -1;

	newnode->element = e;
	newnode->next = NULL;

	if (q->head == NULL) {
		q->head = q->tail = newnode;
	} else {
		q->tail->next = newnode;
		q->tail = newnode;
	}

	return 0;
}

int queue_push(queue_t q, queue_element_t e)
{
	queue_node_ptr_t newnode;


	if (!q)
		return -1;

	newnode = (queue_node_ptr_t) malloc(sizeof(struct queue_node));
	if (newnode == NULL)
		return -1;

	newnode->element = e;
	newnode->next = NULL;

	if (q->head == NULL) {
		q->head = q->tail = newnode;
	} else {
		newnode->next = q->head;
		q->head = newnode;
	}

	return 0;
}

queue_element_t
queue_remove(queue_t q)
{
	queue_node_ptr_t node;
	queue_element_t e;


	if (!q)
		return NULL;

	if (q->head == NULL)
		return NULL;

	node = q->head;
	q->head = q->head->next;
	if (q->head == NULL)
		q->tail = NULL;

	e = node->element;
	free(node);

	return e;
}

queue_element_t
queue_head(queue_t q)
{
	if (!q)
		return NULL;

	if (q->head == NULL)
		return NULL;

	return q->head->element;
}

void queue_destroy(queue_t q)
{
	queue_node_ptr_t p, temp;


	if (!q)
		return;

	p = q->head;
	while (p != NULL) {
		temp = p;
		p = p->next;
		free(temp);
	}

	free(q);
}

int queue_map(queue_t q, int (*f) (queue_element_t, void *), void *vp)
{
	queue_node_ptr_t p;
	int ret;


	if (!q)
		return 0;

	p = q->head;
	while (p != NULL) {
		ret = f(p->element, vp);
		if (ret)
			return ret;
		p = p->next;
	}
	return 0;
}


void queue_map_remove_on_error(queue_t q,
			       int (*f) (queue_element_t, void *),
			       void (*g) (queue_element_t, void *),
			       void *vp)
{
	queue_node_ptr_t p, last, temp;
	int ret;


	if (!q)
		return;

	last = NULL;
	p = q->head;
	while (p != NULL) {
		ret = f(p->element, vp);
		if (ret) {
			if (last) {
				last->next = p->next;
				if (last->next == NULL)
					q->tail = last;
			} else {
				q->head = p->next;
				if (q->head == NULL)
					q->tail = NULL;
			}

			temp = p;
			p = p->next;
			g(temp->element, vp);
			free(temp);
		} else {
			last = p;
			p = p->next;
		}
	}

	return;
}

#endif

/* FLASK */
