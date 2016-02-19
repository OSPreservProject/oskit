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
 * Type definitions for the multi-level security (MLS) policy.
 */

#ifndef _MLS_TYPES_H_
#define _MLS_TYPES_H_

typedef struct mls_level {
	__u32 sens; 	   /* sensitivity */
	ebitmap_t cat;	   /* category set */
} mls_level_t;

typedef struct mls_range {
	mls_level_t level[2]; /* low == level[0], high == level[1] */
} mls_range_t;

typedef struct mls_range_list {
	mls_range_t range;
	struct mls_range_list *next;
} mls_range_list_t;

#define MLS_RELATION_DOM	1 /* source dominates */
#define MLS_RELATION_DOMBY	2 /* target dominates */
#define MLS_RELATION_EQ		4 /* source and target are equivalent */
#define MLS_RELATION_INCOMP	8 /* source and target are incomparable */

#define mls_level_eq(l1,l2) \
(((l1).sens == (l2).sens) && ebitmap_cmp(&(l1).cat,&(l2).cat))

#define mls_level_relation(l1,l2) ( \
(((l1).sens == (l2).sens) && ebitmap_cmp(&(l1).cat,&(l2).cat)) ? \
				    MLS_RELATION_EQ : \
(((l1).sens >= (l2).sens) && ebitmap_contains(&(l1).cat, &(l2).cat)) ? \
				    MLS_RELATION_DOM : \
(((l2).sens >= (l1).sens) && ebitmap_contains(&(l2).cat, &(l1).cat)) ? \
				    MLS_RELATION_DOMBY : \
				    MLS_RELATION_INCOMP )

#define mls_range_contains(r1,r2) \
((mls_level_relation((r1).level[0], (r2).level[0]) & \
	  (MLS_RELATION_EQ | MLS_RELATION_DOMBY)) && \
	 (mls_level_relation((r1).level[1], (r2).level[1]) & \
	  (MLS_RELATION_EQ | MLS_RELATION_DOM)))

/*
 * Every access vector permission is mapped to a set of MLS base
 * permissions, based on the flow properties of the corresponding
 * operation.
 */
typedef struct mls_perms {
	access_vector_t read;     /* permissions that map to `read' */
	access_vector_t readby;   /* permissions that map to `readby' */
	access_vector_t write;    /* permissions that map to `write' */
	access_vector_t writeby;  /* permissions that map to `writeby' */
} mls_perms_t;

#endif

/* FLASK */
