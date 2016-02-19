/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
   hashtab.c

   Implementation of a generic hash table type.
*/
#define NULL ((void *) 0)
#include "hashtab.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#define HASHTAB_DEBUG 0

hashtab_t hashtab_create(unsigned int (*hash_value)(hashtab_key_t key), 
		         int (*keycmp)(hashtab_key_t key1, hashtab_key_t key2),
		         unsigned int size)
{
    hashtab_t p;
    int i;


    MALLOC(p, hashtab_t, sizeof(hashtab_val_t), M_TEMP, M_WAITOK);
    if (p == NULL)
	    return p;

    p->size = size;
    p->hash_value = hash_value;
    p->keycmp = keycmp;
    MALLOC(p->htable, hashtab_ptr_t *, sizeof(hashtab_ptr_t)*size, M_TEMP,M_WAITOK);
    if (p->htable == NULL)
    {
	FREE(p, M_TEMP);
	return NULL;
    }

    for (i = 0; i < size; i++)
	    p->htable[i] = (hashtab_ptr_t) NULL;

    return p;
}


int hashtab_insert(hashtab_t h, hashtab_key_t key, hashtab_datum_t datum)
{
    int hvalue;
    hashtab_ptr_t current, newnode;


    if (!h)
	    return HASHTAB_OVERFLOW;
    
    hvalue = h->hash_value(key);
    current = h->htable[hvalue];
    while (current != NULL && h->keycmp(current->key, key) != 0)
	    current = current->next;

    if (current != NULL)
	    return HASHTAB_PRESENT;

    MALLOC(newnode, hashtab_ptr_t, sizeof(struct hashtab_node_t), M_TEMP, M_WAITOK);
    if (newnode == NULL)
	    return HASHTAB_OVERFLOW;
    newnode->key = key;
    newnode->datum = datum;
    newnode->next = h->htable[hvalue];
    h->htable[hvalue] = newnode;
    return HASHTAB_SUCCESS;
}

int hashtab_remove(hashtab_t h, hashtab_key_t key, 
		   void (*destroy)(hashtab_key_t, hashtab_datum_t, void *),
		   void *args)
{
    int hvalue;
    hashtab_ptr_t current, last;


    if (!h)
	    return HASHTAB_MISSING;
    
    hvalue = h->hash_value(key);
    last = NULL;
    current = h->htable[hvalue];
    while (current != NULL && h->keycmp(current->key, key))
    {
	last = current;
	current = current->next;
    }

    if (current == NULL)
	    return HASHTAB_MISSING;

    if (last == NULL)
	    h->htable[hvalue] = current->next;
    else
	    last->next = current->next;

    if (destroy)
	    destroy(current->key,current->datum,args);
    FREE(current, M_TEMP);
    return HASHTAB_SUCCESS;
}

hashtab_datum_t hashtab_search(hashtab_t h, hashtab_key_t key)
{
    int hvalue;
    hashtab_ptr_t current;


    if (!h)
	    return NULL;
    
    hvalue = h->hash_value(key);
    current = h->htable[hvalue];
    while (current != NULL && h->keycmp(current->key, key))
	    current = current->next;

    if (current == NULL)
	    return NULL;

    return current->datum;
}	    

void hashtab_destroy(hashtab_t h)
{
    int i;
    hashtab_ptr_t current, temp;


    if (!h)
	    return;

    for (i = 0; i < h->size; i++)
	{
	    current = h->htable[i];
	    while (current != NULL)
	    {
		temp = current;
		current = current->next;
		FREE(temp, M_TEMP);
	    }
	}

    FREE(h->htable, M_TEMP);

    FREE(h, M_TEMP);
}

int hashtab_map(hashtab_t h, int (*f)(hashtab_key_t, hashtab_datum_t,void*), void *p)
{
    int i, ret;
    hashtab_ptr_t current;


    if (!h)
	    return HASHTAB_SUCCESS;
    
    for (i = 0; i < h->size; i++)
	{
	    current = h->htable[i];
#if HASHTAB_DEBUG
	    if (current != NULL)
		    printf("\nchecking slot %d of %d.\n", i, h->size);
#endif HASHTAB_DEBUG
	    while (current != NULL)
	    {
		ret = f(current->key, current->datum, p);
		if (ret)
			return ret;
		current = current->next;
	    }
	}
    return HASHTAB_SUCCESS;
}

void hashtab_map_remove_on_error(hashtab_t h, 
				int (*f)(hashtab_key_t,hashtab_datum_t,void*),
				void (*g)(hashtab_key_t,hashtab_datum_t,void*),
				void *p)
{
    int i, ret;
    hashtab_ptr_t last, current, temp;


    if (!h)
	    return;
    
    for (i = 0; i < h->size; i++)
	{
	    last = NULL;
	    current = h->htable[i];
#if HASHTAB_DEBUG
	    if (current != NULL)
		    printf("\nchecking slot %d of %d.\n", i, h->size);
#endif HASHTAB_DEBUG
	    while (current != NULL)
	    {
		ret = f(current->key, current->datum, p);
		if (ret)
		{
		    if (last)
		    {
			last->next = current->next;
		    }
		    else
		    {
			h->htable[i] = current->next;
		    }
		    
		    temp = current;
		    current = current->next;
		    g(temp->key, temp->datum, p);
		    FREE(temp, M_TEMP);
		}
		else
		{
		    last = current;
		    current = current->next;
		}
	    }
	}

    return;
}

