/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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

#ifndef _THREADS_CPUI_HASH_
#define _THREADS_CPUI_HASH_

#include <oskit/threads/pthread.h>

#define DEFAULT_HASHTAB_SIZE	63

/*
 * Each hash element.
 */
typedef struct hash_element {
	queue_chain_t		chain;
	void			*item;
	pthread_t		key;
} hash_element_t;

/*
 * The hash table.
 */
typedef struct hash_table {
	int			size;		/* Size of the table */
	int			count;		/* Number of items in table */
	spin_lock_t		lock;		/* Mutex is too heavyweight */
	queue_head_t		buckets[0];	/* A list of queues */
} hash_table_t;

oskit_error_t   tidhash_create(hash_table_t **table, int size);
void		tidhash_destroy(hash_table_t *table);
oskit_error_t	tidhash_add(hash_table_t *table, void *item, pthread_t key);
oskit_error_t	tidhash_rem(hash_table_t *table, pthread_t key);
void	       *tidhash_lookup(hash_table_t *table, pthread_t key);

#endif /* _THREADS_CPUI_HASH_ */
