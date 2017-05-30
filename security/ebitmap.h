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
 * An extensible bitmap is a bitmap that supports an 
 * arbitrary number of bits.  Extensible bitmaps are
 * used to represent sets of values, such as types,
 * roles, categories, and classes.
 *
 * Each extensible bitmap is implemented as a linked
 * list of bitmap nodes, where each bitmap node has
 * an explicitly specified starting bit position within
 * the total bitmap.
 */

#ifndef _EBITMAP_H_
#define _EBITMAP_H_

#define MAPTYPE __u64			/* portion of bitmap in each node */
#define MAPSIZE (sizeof(MAPTYPE) * 8)	/* number of bits in node bitmap */
#define MAPBIT  1ULL			/* a bit in the node bitmap */

typedef struct ebitmap_node {
	__u32 startbit;		/* starting position in the total bitmap */
	MAPTYPE map;		/* this node's portion of the bitmap */
	struct ebitmap_node *next;
} ebitmap_node_t;

typedef struct ebitmap {
	ebitmap_node_t *node;	/* first node in the bitmap */
	__u32 highbit;	/* highest position in the total bitmap */
} ebitmap_t;


#define ebitmap_length(e) ((e)->highbit)

#define ebitmap_init(e) memset(e, 0, sizeof(ebitmap_t))

/*
 * All of the non-void functions return TRUE or FALSE.
 * Contrary to typical usage, nonzero (TRUE) is returned
 * on success and zero (FALSE) is returned on failure.
 * These functions should be changed to use more conventional 
 * return codes.  TBD.
 */
#define FALSE 0
#define TRUE 1

int ebitmap_cmp(ebitmap_t * e1, ebitmap_t * e2);
int ebitmap_or(ebitmap_t * dst, ebitmap_t * e1, ebitmap_t * e2);
int ebitmap_cpy(ebitmap_t * dst, ebitmap_t * src);
int ebitmap_contains(ebitmap_t * e1, ebitmap_t * e2);
int ebitmap_get_bit(ebitmap_t * e, unsigned long bit);
int ebitmap_set_bit(ebitmap_t * e, unsigned long bit, int value);
void ebitmap_destroy(ebitmap_t * e);
int ebitmap_read(ebitmap_t * e, FILE * fp);
#ifndef __KERNEL__
int ebitmap_write(ebitmap_t * e, FILE * fp);
#endif

#endif	/* _EBITMAP_H_ */

/* FLASK */
