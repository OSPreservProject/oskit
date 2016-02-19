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

/* 
 * A double-ended queue is a singly linked list of 
 * elements of arbitrary type that may be accessed
 * at either end.
 */

#ifndef _QUEUE_H_
#define _QUEUE_H_

typedef void *queue_element_t;

typedef struct queue_node *queue_node_ptr_t;

typedef struct queue_node {
	queue_element_t element;
	queue_node_ptr_t next;
} queue_node_t;

typedef struct queue_info {
	queue_node_ptr_t head;
	queue_node_ptr_t tail;
} queue_info_t;

typedef queue_info_t *queue_t;

queue_t queue_create(void);
int queue_insert(queue_t, queue_element_t);
int queue_push(queue_t, queue_element_t);
queue_element_t queue_remove(queue_t);
queue_element_t queue_head(queue_t);
void queue_destroy(queue_t);

/* 
   Applies the specified function f to each element in the
   specified queue. 

   In addition to passing the element to f, queue_map
   passes the specified void* pointer to f on each invocation.

   If f returns a non-zero status, then queue_map will cease
   iterating through the hash table and will propagate the error
   return to its caller.
 */
int queue_map(queue_t, int (*f) (queue_element_t, void *), void *);

/*
   Same as queue_map, except that if f returns a non-zero status,
   then the element will be removed from the queue and the g
   function will be applied to the element. 
 */
void queue_map_remove_on_error(queue_t,
			       int (*f) (queue_element_t, void *),
			       void (*g) (queue_element_t, void *),
			       void *);

#endif


/* FLASK */
