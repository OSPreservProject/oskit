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
 * An access vector table (avtab) is a hash table
 * of access vectors and transition types indexed 
 * by a type pair and a class.  An access vector
 * table is used to represent the type enforcement
 * tables.
 */

#ifndef _AVTAB_H_
#define _AVTAB_H_

typedef struct avtab_key {
	__u32 source_type;	/* source type */
	__u32 target_type;	/* target type */
	__u32 target_class;	/* class of target object */
} avtab_key_t;

typedef struct avtab_datum {
#define AVTAB_ALLOWED     1
#define AVTAB_AUDITALLOW  2
#define AVTAB_AUDITDENY   4
#define AVTAB_NOTIFY      8
#define AVTAB_TRANSITION 16
	__u32 specified;			/* what fields are specified */

	access_vector_t allowed;	/* allowed permissions */
	__u32 trans_type;			/* transition type */

#ifdef CONFIG_FLASK_AUDIT
	access_vector_t auditallow;	/* audit when granted */
	access_vector_t auditdeny;	/* audit when denied */
#endif

#ifdef CONFIG_FLASK_NOTIFY
	access_vector_t notify;	/* notify when completed */
#endif
} avtab_datum_t;

typedef struct avtab_node *avtab_ptr_t;

struct avtab_node {
	avtab_key_t key;
	avtab_datum_t datum;
	avtab_ptr_t next;
};

#define AVTAB_SIZE 1013

typedef struct avtab {
	avtab_ptr_t htable[AVTAB_SIZE];
	__u32 nel;	/* number of elements */
} avtab_t;

int avtab_init(avtab_t *);

int avtab_insert(avtab_t * h, avtab_key_t * k, avtab_datum_t * d);

avtab_datum_t *avtab_search(avtab_t * h, avtab_key_t * k);

void avtab_destroy(avtab_t * h);

int avtab_map(avtab_t * h,
	      int (*apply) (avtab_key_t * k,
			    avtab_datum_t * d,
			    void *args),
	      void *args);

void avtab_hash_eval(avtab_t * h, char *progname, char *table);

int avtab_read(avtab_t * a, FILE * fp, __u32 config);

#ifndef __KERNEL__
int avtab_write(avtab_t * a, FILE * fp);
#endif

#endif	/* _AVTAB_H_ */

/* FLASK */
