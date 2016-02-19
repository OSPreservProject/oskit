/*
 * Copyright (c) 1997,1999 The University of Utah and the Flux Group.
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
   hashtab.h

   Definition of a generic hash table type.
   
   A hash table establishes an association between a set
   of key values and a set of datum values.  Each entry in
   the hash table is a (key,datum) pair.  Keys must uniquely
   identify an entry in the hash table.

   Keys and datums are pointers to objects of any type.  
   The datum types may vary for different objects within
   the same hash table.  

   Collision resolution is performed using chaining.
*/

#ifndef _OSKIT_COM_SFS_HASHTAB_H_
#define _OSKIT_COM_SFS_HASHTAB_H_

typedef char* hashtab_key_t;		       /* generic key type */
typedef void* hashtab_datum_t;		       /* generic datum type */

/* 
  A node in a hash table is a (key,datum) pair.  
*/
typedef struct hashtab_node_t* hashtab_ptr_t;

struct hashtab_node_t
{
    hashtab_key_t key;
    hashtab_datum_t datum;
    hashtab_ptr_t next;			       /* link to next node in chain */
};


/* 
  A hash table is implemented as a record containing:
         an array of pointers to chains of nodes (htable)
	 the number of slots in the array (size)
	 the hash function to be used (hash_value)
	 the comparison function for keys (keycmp)

  The two functions, hash_value and keycmp, are defined when
  a new hash table is created so that the hash table code need
  not depend upon the types of the keys.

  keycmp is expected to have the same semantics as strcmp,
  although the implementation only requires the ability to
  distinguish between equal keys and nonequal keys.
*/
typedef struct 
{
    hashtab_ptr_t *htable;
    unsigned int size;
    unsigned int (*hash_value)(hashtab_key_t key);
    int (*keycmp)(hashtab_key_t key1, hashtab_key_t key2);
} hashtab_val_t;


typedef hashtab_val_t* hashtab_t;

/* Define status codes for hash table functions */
#include <oskit/error.h>
#define HASHTAB_SUCCESS     0		
#define HASHTAB_OVERFLOW    OSKIT_ENOMEM
#define HASHTAB_PRESENT     OSKIT_EEXIST
#define HASHTAB_MISSING     OSKIT_ENOENT


/* Symbol space cleanliness */
#define hashtab_create oskit_com_sfs_hashtab_create
#define hashtab_destroy oskit_com_sfs_hashtab_destroy
#define hashtab_insert oskit_com_sfs_hashtab_insert
#define hashtab_remove oskit_com_sfs_hashtab_remove
#define hashtab_replace oskit_com_sfs_hashtab_replace
#define hashtab_search oskit_com_sfs_hashtab_search
#define hashtab_map oskit_com_sfs_hashtab_map
#define hashtab_map_remove_on_error oskit_com_sfs_hashtab_map_remove_on_error


/* 
  Creates a new hash table with the specified characteristics.

  Returns NULL if insufficent space is available or 
  the new hash table otherwise.
*/
hashtab_t hashtab_create(unsigned int (*hash_value)(hashtab_key_t key), 
			 int (*keycmp)(hashtab_key_t key1, 
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
		   void (*destroy)(hashtab_key_t k, 
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
		   void (*destroy)(hashtab_key_t k, 
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

  Note that this function does not destroy the keys and datums
  stored in the specified hash table, since keys and datums were
  allocated by the caller before insertion.
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
		int (*apply)(hashtab_key_t k, 
			     hashtab_datum_t d,
			     void *args), 
		void *args);

/*
  Same as hashtab_map, except that if apply returns a non-zero status,
  then the (key,datum) pair will be removed from the hashtab and the
  destroy function will be applied to (key,datum,args).
 */
void hashtab_map_remove_on_error(hashtab_t h, 
				int (*apply)(hashtab_key_t k,
					     hashtab_datum_t d,
					     void *args),
				void (*destroy)(hashtab_key_t k,
						hashtab_datum_t d,
						void *args),
				void *args);


#endif
