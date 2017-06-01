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

#ifdef	CPU_INHERIT
/*
 * Simple hash table used by schedulers to map pthread TIDs to a data value. 
 */

#include <stdlib.h>
#include <oskit/error.h>
#include <oskit/queue.h>
#include "hash.h"

/*
 * Hash function maps tid to index.
 */
static inline int
hashfunc(pthread_t key, hash_table_t *table)
{
	return (((int) key) % (table->size));
}

oskit_error_t
tidhash_create(hash_table_t **table, int size)
{
	hash_table_t	*ptmp;
	int		i;
	
	if (size == 0)
		size = DEFAULT_HASHTAB_SIZE;

	if ((ptmp =
	     (hash_table_t *) malloc(sizeof(hash_table_t) +
				     sizeof(queue_head_t) * size)) == NULL)
		return OSKIT_ENOMEM;

	ptmp->count = 0;
	ptmp->size  = size;
	spin_lock_init(&ptmp->lock);
	for (i = 0; i < size; i++)
		queue_init(&ptmp->buckets[i]);

	*table = ptmp;
	return 0;
}

void
tidhash_destroy(hash_table_t *table)
{
	int		i;
	hash_element_t	*entry;

	spin_lock(&table->lock);
	
	for (i = 0; i < table->size; i++) {
		queue_head_t	*q = &table->buckets[i];

		while (! queue_empty(q)) {
			queue_remove_first(q, entry, hash_element_t *, chain);
			free(entry);
		}
	}
	free(table);
}

oskit_error_t
tidhash_add(hash_table_t *table, void *item, pthread_t key)
{
	int		index = hashfunc(key, table);
	queue_head_t	*q    = &table->buckets[index];
	hash_element_t  *entry;

	if ((entry = (hash_element_t *) malloc(sizeof(*entry))) == NULL)
		return OSKIT_ENOMEM;

	entry->key  = key;
	entry->item = item;
	
	spin_lock(&table->lock);
	queue_enter(q, entry, hash_element_t *, chain);
	spin_unlock(&table->lock);
	
	return 0;
}

oskit_error_t
tidhash_rem(hash_table_t *table, pthread_t key)
{
	int		index  = hashfunc(key, table);
	queue_head_t	*q     = &table->buckets[index];
	hash_element_t  *entry;

	spin_lock(&table->lock);
	queue_iterate(q, entry, hash_element_t *, chain) {
		if (entry->key == key) {
			queue_remove(q, entry, hash_element_t *, chain);
			spin_unlock(&table->lock);
			free(entry);
			return 0;
		}
	}
	panic("tidhash_rem: No element for key %d in table %x\n",
	      (int) key, table);
}

void *
tidhash_lookup(hash_table_t *table, pthread_t key)
{
	int		index  = hashfunc(key, table);
	queue_head_t	*q     = &table->buckets[index];
	hash_element_t  *entry;

	spin_lock(&table->lock);
	entry = (hash_element_t *) queue_first(q);
	if (entry->key == key) {
		spin_unlock(&table->lock);
		return entry->item;
	}
	    
	queue_iterate(q, entry, hash_element_t *, chain) {
		if (entry->key == key) {
			spin_unlock(&table->lock);
			return entry->item;
		}
	}
	spin_unlock(&table->lock);
	return (void *) 0;
}
#endif
