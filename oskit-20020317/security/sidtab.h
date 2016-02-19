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
 * A security identifier table (sidtab) is a hash table
 * of security context structures indexed by SID value.
 */

#ifndef _SIDTAB_H_
#define _SIDTAB_H_

#include "context.h"

typedef struct sidtab_node {
	security_id_t sid;		/* security identifier */
	context_struct_t context;	/* security context structure */
	struct sidtab_node *next;
} sidtab_node_t;

#define SIDTAB_SIZE 23

typedef struct {
	sidtab_node_t *htable[SIDTAB_SIZE];
	unsigned int nel;	/* number of elements */
} sidtab_t;


int sidtab_insert(sidtab_t * s, security_id_t sid, context_struct_t * context);

context_struct_t *sidtab_search(sidtab_t * s, security_id_t sid);

int sidtab_map(sidtab_t * s,
	       int (*apply) (security_id_t sid,
			     context_struct_t * context,
			     void *args),
	       void *args);

void sidtab_map_remove_on_error(sidtab_t * s,
				int (*apply) (security_id_t sid,
					      context_struct_t * context,
					      void *args),
				void *args);

int sidtab_context_to_sid(sidtab_t * s,		/* IN */
			  context_struct_t * context,	/* IN */
			  security_id_t * sid);		/* OUT */

#endif	/* _SIDTAB_H_ */

/* FLASK */
