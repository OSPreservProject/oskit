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
 * Implementation of the hash table type.
 */

#include "hashtab.h"

#define HASHTAB_DEBUG 0

hashtab_t hashtab_create(unsigned int (*hash_value) (hashtab_key_t key),
			 int (*keycmp) (hashtab_key_t key1,
					hashtab_key_t key2),
			 unsigned int size)
{
	hashtab_t p;
	int i;


	p = (hashtab_t) malloc(sizeof(hashtab_val_t));
	if (p == NULL)
		return p;

	memset(p, 0, sizeof(hashtab_val_t));
	p->size = size;
	p->nel = 0;
	p->hash_value = hash_value;
	p->keycmp = keycmp;
	p->htable = (hashtab_ptr_t *) malloc(sizeof(hashtab_ptr_t) * size);
	if (p->htable == NULL) {
		free(p);
		return NULL;
	}
	for (i = 0; i < size; i++)
		p->htable[i] = (hashtab_ptr_t) NULL;

	return p;
}


int hashtab_insert(hashtab_t h, hashtab_key_t key, hashtab_datum_t datum)
{
	int hvalue;
	hashtab_ptr_t cur, newnode;


	if (!h)
		return HASHTAB_OVERFLOW;

	hvalue = h->hash_value(key);
	cur = h->htable[hvalue];
	while (cur != NULL && h->keycmp(cur->key, key) != 0)
		cur = cur->next;

	if (cur != NULL)
		return HASHTAB_PRESENT;

	newnode = (hashtab_ptr_t) malloc(sizeof(hashtab_node_t));
	if (newnode == NULL)
		return HASHTAB_OVERFLOW;
	memset(newnode, 0, sizeof(struct hashtab_node));
	newnode->key = key;
	newnode->datum = datum;
	newnode->next = h->htable[hvalue];
	h->htable[hvalue] = newnode;
	h->nel++;
	return HASHTAB_SUCCESS;
}


int hashtab_remove(hashtab_t h, hashtab_key_t key,
		   void (*destroy) (hashtab_key_t k,
				    hashtab_datum_t d,
				    void *args),
		   void *args)
{
	int hvalue;
	hashtab_ptr_t cur, last;


	if (!h)
		return HASHTAB_MISSING;

	hvalue = h->hash_value(key);
	last = NULL;
	cur = h->htable[hvalue];
	while (cur != NULL && h->keycmp(cur->key, key)) {
		last = cur;
		cur = cur->next;
	}

	if (cur == NULL)
		return HASHTAB_MISSING;

	if (last == NULL)
		h->htable[hvalue] = cur->next;
	else
		last->next = cur->next;

	if (destroy)
		destroy(cur->key, cur->datum, args);
	free(cur);
	h->nel--;
	return HASHTAB_SUCCESS;
}


int hashtab_replace(hashtab_t h, hashtab_key_t key, hashtab_datum_t datum,
		    void (*destroy) (hashtab_key_t k,
				     hashtab_datum_t d,
				     void *args),
		    void *args)
{
	int hvalue;
	hashtab_ptr_t cur, newnode;


	if (!h)
		return HASHTAB_OVERFLOW;

	hvalue = h->hash_value(key);
	cur = h->htable[hvalue];
	while (cur != NULL && h->keycmp(cur->key, key) != 0)
		cur = cur->next;

	if (cur) {
		if (destroy)
			destroy(cur->key, cur->datum, args);
		cur->key = key;
		cur->datum = datum;
	} else {
		newnode = (hashtab_ptr_t) malloc(sizeof(hashtab_node_t));
		if (newnode == NULL)
			return HASHTAB_OVERFLOW;
		memset(newnode, 0, sizeof(struct hashtab_node));
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
	int hvalue;
	hashtab_ptr_t cur;


	if (!h)
		return NULL;

	hvalue = h->hash_value(key);
	cur = h->htable[hvalue];
	while (cur != NULL && h->keycmp(cur->key, key))
		cur = cur->next;

	if (cur == NULL)
		return NULL;

	return cur->datum;
}


void hashtab_destroy(hashtab_t h)
{
	int i;
	hashtab_ptr_t cur, temp;


	if (!h)
		return;

	for (i = 0; i < h->size; i++) {
		cur = h->htable[i];
		while (cur != NULL) {
			temp = cur;
			cur = cur->next;
			free(temp);
		}
		h->htable[i] = NULL;
	}

	free(h->htable);
	h->htable = NULL;

	free(h);
}


int hashtab_map(hashtab_t h,
		int (*apply) (hashtab_key_t k,
			      hashtab_datum_t d,
			      void *args),
		void *args)
{
	int i, ret;
	hashtab_ptr_t cur;


	if (!h)
		return HASHTAB_SUCCESS;

	for (i = 0; i < h->size; i++) {
		cur = h->htable[i];
#if HASHTAB_DEBUG
		if (cur != NULL)
			printf("\nchecking slot %d of %d.\n", i, h->size);
#endif				/* HASHTAB_DEBUG */
		while (cur != NULL) {
			ret = apply(cur->key, cur->datum, args);
			if (ret)
				return ret;
			cur = cur->next;
		}
	}
	return HASHTAB_SUCCESS;
}


void hashtab_map_remove_on_error(hashtab_t h,
				 int (*apply) (hashtab_key_t k,
					       hashtab_datum_t d,
					       void *args),
				 void (*destroy) (hashtab_key_t k,
						  hashtab_datum_t d,
						  void *args),
				 void *args)
{
	int i, ret;
	hashtab_ptr_t last, cur, temp;


	if (!h)
		return;

	for (i = 0; i < h->size; i++) {
		last = NULL;
		cur = h->htable[i];
#if HASHTAB_DEBUG
		if (cur != NULL)
			printf("\nchecking slot %d of %d.\n", i, h->size);
#endif				/* HASHTAB_DEBUG */
		while (cur != NULL) {
			ret = apply(cur->key, cur->datum, args);
			if (ret) {
				if (last) {
					last->next = cur->next;
				} else {
					h->htable[i] = cur->next;
				}

				temp = cur;
				cur = cur->next;
				if (destroy)
					destroy(temp->key, temp->datum, args);
				free(temp);
				h->nel--;
			} else {
				last = cur;
				cur = cur->next;
			}
		}
	}

	return;
}
