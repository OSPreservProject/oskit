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
#ifndef _OSKIT_COM_SFS_NEW_HASHTAB_H_
#define _OSKIT_COM_SFS_NEW_HASHTAB_H_

#include <oskit/types.h>
#include <oskit/com.h>
#include <oskit/com/services.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_mem.h>

typedef oskit_u32_t hashtab_key_t;	
typedef oskit_u32_t hashtab_datum_t;	

#define INOTAB_SIZE 71
#define HASH_VALUE(k) ((k) % INOTAB_SIZE)
#define KEYCMP(k1,k2) ((k1) != (k2))

typedef struct hashtab_node_t* hashtab_ptr_t;

struct hashtab_node_t
{
    hashtab_key_t key;
    hashtab_datum_t datum;
    hashtab_ptr_t next;
};


typedef struct 
{
    hashtab_ptr_t *htable;
    oskit_osenv_mem_t *mem;
} hashtab_val_t;


typedef hashtab_val_t* hashtab_t;

#include <oskit/error.h>
#define HASHTAB_SUCCESS     0		
#define HASHTAB_OVERFLOW    OSKIT_ENOMEM
#define HASHTAB_PRESENT     OSKIT_EEXIST
#define HASHTAB_MISSING     OSKIT_ENOENT


/* Symbol space cleanliness */
#define hashtab_create oskit_com_sfs_new_hashtab_create
#define hashtab_destroy oskit_com_sfs_new_hashtab_destroy
#define hashtab_insert oskit_com_sfs_new_hashtab_insert
#define hashtab_remove oskit_com_sfs_new_hashtab_remove
#define hashtab_replace oskit_com_sfs_new_hashtab_replace
#define hashtab_search oskit_com_sfs_new_hashtab_search
#define hashtab_reverse_search oskit_com_sfs_new_hashtab_reverse_search
#define hashtab_map oskit_com_sfs_new_hashtab_map
#define hashtab_map_remove_on_error oskit_com_sfs_new_hashtab_map_remove_on_error

hashtab_t hashtab_create(oskit_osenv_mem_t *mem);
int hashtab_insert(hashtab_t h, hashtab_key_t k, hashtab_datum_t d);
int hashtab_remove(hashtab_t h, hashtab_key_t k);
int hashtab_replace(hashtab_t h, hashtab_key_t k, hashtab_datum_t d);
hashtab_datum_t hashtab_search(hashtab_t h, hashtab_key_t k);
hashtab_key_t hashtab_reverse_search(hashtab_t h, hashtab_datum_t d);
void hashtab_destroy(hashtab_t h);
int hashtab_map(hashtab_t h, 
		int (*apply)(hashtab_key_t k, 
			     hashtab_datum_t d,
			     void *args), 
		void *args);
void hashtab_map_remove_on_error(hashtab_t h, 
				int (*apply)(hashtab_key_t k,
					     hashtab_datum_t d,
					     void *args),
				void *args);

#endif
