/*
 * Copyright (c) 1999 The University of Utah and the Flux Group. 
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
#define NULL ((void *) 0)
#include "sfs_new_hashtab.h"

#define HASHTAB_DEBUG 0

hashtab_t 
hashtab_create(oskit_osenv_mem_t *mem)
{
	hashtab_t       p;
	int             i;


	p = (hashtab_t) oskit_osenv_mem_alloc(mem,sizeof(hashtab_val_t),0,0);
	if (p == NULL)
		return p;

	p->htable = (hashtab_ptr_t *) 
	  oskit_osenv_mem_alloc(mem,sizeof(hashtab_ptr_t)*INOTAB_SIZE,0,0);
	if (p->htable == NULL) {
		oskit_osenv_mem_free(mem, p, 0, sizeof(hashtab_val_t));
		return NULL;
	}
	for (i = 0; i < INOTAB_SIZE; i++)
		p->htable[i] = (hashtab_ptr_t) NULL;

	p->mem = mem;
	oskit_osenv_mem_addref(mem);

	return p;
}


int 
hashtab_insert(hashtab_t h, hashtab_key_t key, hashtab_datum_t datum)
{
	int             hvalue;
	hashtab_ptr_t   current, newnode;


	if (!h)
		return HASHTAB_OVERFLOW;

	hvalue = HASH_VALUE(key);
	current = h->htable[hvalue];
	while (current != NULL && KEYCMP(current->key, key))
		current = current->next;

	if (current != NULL)
		return HASHTAB_PRESENT;

	newnode = (hashtab_ptr_t) 
		oskit_osenv_mem_alloc(h->mem,
				      sizeof(struct hashtab_node_t),0,0);
	if (newnode == NULL)
		return HASHTAB_OVERFLOW;
	newnode->key = key;
	newnode->datum = datum;
	newnode->next = h->htable[hvalue];
	h->htable[hvalue] = newnode;
	return HASHTAB_SUCCESS;
}


int 
hashtab_remove(hashtab_t h, hashtab_key_t key)
{
	int             hvalue;
	hashtab_ptr_t   current, last;


	if (!h)
		return HASHTAB_MISSING;

	hvalue = HASH_VALUE(key);
	last = NULL;
	current = h->htable[hvalue];
	while (current != NULL && KEYCMP(current->key, key)) {
		last = current;
		current = current->next;
	}

	if (current == NULL)
		return HASHTAB_MISSING;

	if (last == NULL)
		h->htable[hvalue] = current->next;
	else
		last->next = current->next;

	oskit_osenv_mem_free(h->mem, current, 0, 
			     sizeof(struct hashtab_node_t));
	return HASHTAB_SUCCESS;
}


int 
hashtab_replace(hashtab_t h, hashtab_key_t key, hashtab_datum_t datum)
{
	int             hvalue;
	hashtab_ptr_t   current, newnode;


	if (!h)
		return HASHTAB_OVERFLOW;

	hvalue = HASH_VALUE(key);
	current = h->htable[hvalue];
	while (current != NULL && KEYCMP(current->key, key) != 0)
		current = current->next;

	if (current) {
		current->key = key;
		current->datum = datum;
	} else {
		newnode = (hashtab_ptr_t) 
			oskit_osenv_mem_alloc(h->mem,
				      sizeof(struct hashtab_node_t),0,0);
		if (newnode == NULL)
			return HASHTAB_OVERFLOW;
		newnode->key = key;
		newnode->datum = datum;
		newnode->next = h->htable[hvalue];
		h->htable[hvalue] = newnode;
	}

	return HASHTAB_SUCCESS;
}


hashtab_datum_t 
hashtab_search(hashtab_t h, hashtab_key_t key)
{
	int             hvalue;
	hashtab_ptr_t   current;


	if (!h)
		return 0;

	hvalue = HASH_VALUE(key);
	current = h->htable[hvalue];
	while (current != NULL && KEYCMP(current->key, key))
		current = current->next;

	if (current == NULL)
		return 0;

	return current->datum;
}


hashtab_key_t 
hashtab_reverse_search(hashtab_t h, hashtab_datum_t datum)
{
	int             i;
	hashtab_ptr_t   current;


	if (!h)
		return 0;

	for (i = 0; i < INOTAB_SIZE; i++) {
		current = h->htable[i];
		while (current != NULL) {
			if (current->key && (current->datum == datum))
				return current->key;
			current = current->next;
		}
	}
	return 0;
}


void 
hashtab_destroy(hashtab_t h)
{
	int             i;
	hashtab_ptr_t   current, temp;
	oskit_osenv_mem_t *mem;


	if (!h)
		return;

	for (i = 0; i < INOTAB_SIZE; i++) {
		current = h->htable[i];
		while (current != NULL) {
			temp = current;
			current = current->next;
			oskit_osenv_mem_free(h->mem, temp, 0, 
					     sizeof(struct hashtab_node_t));
		}
	}

	oskit_osenv_mem_free(h->mem, h->htable, 0, 
			     sizeof(hashtab_ptr_t)*INOTAB_SIZE);
	mem = h->mem;
	oskit_osenv_mem_free(mem, h, 0, sizeof(hashtab_val_t));
	oskit_osenv_mem_release(mem);
}


int 
hashtab_map(hashtab_t h,
	    int (*apply) (hashtab_key_t k,
			  hashtab_datum_t d,
			  void *args),
	    void *args)
{
	int             i, ret;
	hashtab_ptr_t   current;


	if (!h)
		return HASHTAB_SUCCESS;

	for (i = 0; i < INOTAB_SIZE; i++) {
		current = h->htable[i];
		while (current != NULL) {
			ret = apply(current->key, current->datum, args);
			if (ret)
				return ret;
			current = current->next;
		}
	}
	return HASHTAB_SUCCESS;
}


void 
hashtab_map_remove_on_error(hashtab_t h,
			    int (*apply) (hashtab_key_t k,
					  hashtab_datum_t d,
					  void *args),
			    void *args)
{
	int             i, ret;
	hashtab_ptr_t   last, current, temp;


	if (!h)
		return;

	for (i = 0; i < INOTAB_SIZE; i++) {
		last = NULL;
		current = h->htable[i];
		while (current != NULL) {
			ret = apply(current->key, current->datum, args);
			if (ret) {
				if (last) {
					last->next = current->next;
				} else {
					h->htable[i] = current->next;
				}

				temp = current;
				current = current->next;
				oskit_osenv_mem_free(h->mem, temp, 0, 
					     sizeof(struct hashtab_node_t));
			} else {
				last = current;
				current = current->next;
			}
		}
	}

	return;
}
