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
 * A hash table (hashtab) maintains associations between
 * key values and datum values.  The type of the key values 
 * and the type of the datum values is arbitrary.  The
 * functions for hash computation and key comparison are
 * provided by the creator of the table.
 */

#ifndef _HASHTAB_H_
#define _HASHTAB_H_

typedef char *hashtab_key_t;	/* generic key type */
typedef void *hashtab_datum_t;	/* generic datum type */

typedef struct hashtab_node *hashtab_ptr_t;

typedef struct hashtab_node {
	hashtab_key_t key;
	hashtab_datum_t datum;
	hashtab_ptr_t next;	
} hashtab_node_t;

typedef struct hashtab_val {
	hashtab_ptr_t *htable; /* hash table */
	unsigned int size; /* number of slots in hash table */
	__u32 nel;  	  /* number of elements in hash table */
	unsigned int (*hash_value) (hashtab_key_t key); /* hash function */
	int (*keycmp) (hashtab_key_t key1, hashtab_key_t key2); /* key comparison function */
} hashtab_val_t;


typedef hashtab_val_t *hashtab_t;

/* Define status codes for hash table functions */
#define HASHTAB_SUCCESS     0
#define HASHTAB_OVERFLOW    -ENOMEM
#define HASHTAB_PRESENT     -EEXIST
#define HASHTAB_MISSING     -ENOENT

/*
   Creates a new hash table with the specified characteristics.

   Returns NULL if insufficent space is available or
   the new hash table otherwise.
 */
hashtab_t hashtab_create(unsigned int (*hash_value) (hashtab_key_t key),
			 int (*keycmp) (hashtab_key_t key1,
					hashtab_key_t key2),
			 unsigned int size);

/*
   Inserts the specified (key, datum) pair into the specified hash table.

   Returns HASHTAB_OVERFLOW if insufficient space is available or
   HASHTAB_PRESENT  if there is already an entry with the same key or
   HASHTAB_SUCCESS otherwise.
 */
int hashtab_insert(hashtab_t h, hashtab_key_t k, hashtab_datum_t d);

/*
   Removes the entry with the specified key from the hash table.
   Applies the specified destroy function to (key,datum,args) for
   the entry.

   Returns HASHTAB_MISSING if no entry has the specified key or
   HASHTAB_SUCCESS otherwise.
 */
int hashtab_remove(hashtab_t h, hashtab_key_t k,
		   void (*destroy) (hashtab_key_t k,
				    hashtab_datum_t d,
				    void *args),
		   void *args);

/*
   Insert or replace the specified (key, datum) pair in the specified
   hash table.  If an entry for the specified key already exists,
   then the specified destroy function is applied to (key,datum,args)
   for the entry prior to replacing the entry's contents.

   Returns HASHTAB_OVERFLOW if insufficient space is available or
   HASHTAB_SUCCESS otherwise.
 */
int hashtab_replace(hashtab_t h, hashtab_key_t k, hashtab_datum_t d,
		    void (*destroy) (hashtab_key_t k,
				     hashtab_datum_t d,
				     void *args),
		    void *args);

/*
   Searches for the entry with the specified key in the hash table.

   Returns NULL if no entry has the specified key or
   the datum of the entry otherwise.
 */
hashtab_datum_t hashtab_search(hashtab_t h, hashtab_key_t k);

/*
   Destroys the specified hash table.
 */
void hashtab_destroy(hashtab_t h);

/*
   Applies the specified apply function to (key,datum,args)
   for each entry in the specified hash table.

   The order in which the function is applied to the entries
   is dependent upon the internal structure of the hash table.

   If apply returns a non-zero status, then hashtab_map will cease
   iterating through the hash table and will propagate the error
   return to its caller.
 */
int hashtab_map(hashtab_t h,
		int (*apply) (hashtab_key_t k,
			      hashtab_datum_t d,
			      void *args),
		void *args);

/*
   Same as hashtab_map, except that if apply returns a non-zero status,
   then the (key,datum) pair will be removed from the hashtab and the
   destroy function will be applied to (key,datum,args).
 */
void hashtab_map_remove_on_error(hashtab_t h,
				 int (*apply) (hashtab_key_t k,
					       hashtab_datum_t d,
					       void *args),
				 void (*destroy) (hashtab_key_t k,
						  hashtab_datum_t d,
						  void *args),
				 void *args);


#endif
