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
 * Implementation of the SID table type.
 */

#include "sidtab.h"
#include "services.h"

#define SIDTAB_HASH(sid) \
(sid % SIDTAB_SIZE)


int sidtab_insert(sidtab_t * s, security_id_t sid, context_struct_t * context)
{
	int hvalue;
	sidtab_node_t *cur, *newnode;


	if (!s)
		return -ENOMEM;

	hvalue = SIDTAB_HASH(sid);
	cur = s->htable[hvalue];
	while (cur != NULL && cur->sid != sid)
		cur = cur->next;

	if (cur != NULL)
		return -EEXIST;

	newnode = (sidtab_node_t *) malloc(sizeof(sidtab_node_t));
	if (newnode == NULL)
		return -ENOMEM;
	newnode->sid = sid;
	if (context_cpy(&newnode->context, context)) {
		free(newnode);
		return -ENOMEM;
	}
	newnode->next = s->htable[hvalue];
	s->htable[hvalue] = newnode;

	s->nel++;
	return 0;
}


int sidtab_remove(sidtab_t * s, security_id_t sid)
{
	int hvalue;
	sidtab_node_t *cur, *last;


	if (!s)
		return -ENOENT;

	hvalue = SIDTAB_HASH(sid);
	last = NULL;
	cur = s->htable[hvalue];
	while (cur != NULL && cur->sid != sid) {
		last = cur;
		cur = cur->next;
	}

	if (cur == NULL)
		return -ENOENT;

	if (last == NULL)
		s->htable[hvalue] = cur->next;
	else
		last->next = cur->next;

	context_destroy(&cur->context);

	free(cur);
	return 0;
}


context_struct_t *
 sidtab_search(sidtab_t * s, security_id_t sid)
{
	int hvalue;
	sidtab_node_t *cur;


	if (!s)
		return NULL;

	hvalue = SIDTAB_HASH(sid);
	cur = s->htable[hvalue];
	while (cur != NULL && cur->sid != sid)
		cur = cur->next;

	if (cur == NULL)
		return NULL;

	return &cur->context;
}


int sidtab_map(sidtab_t * s,
	       int (*apply) (security_id_t sid,
			     context_struct_t * context,
			     void *args),
	       void *args)
{
	int i, ret;
	sidtab_node_t *cur;


	if (!s)
		return 0;

	for (i = 0; i < SIDTAB_SIZE; i++) {
		cur = s->htable[i];
		while (cur != NULL) {
			ret = apply(cur->sid, &cur->context, args);
			if (ret)
				return ret;
			cur = cur->next;
		}
	}
	return 0;
}


void sidtab_map_remove_on_error(sidtab_t * s,
				int (*apply) (security_id_t sid,
					      context_struct_t * context,
					      void *args),
				void *args)
{
	int i, ret;
	sidtab_node_t *last, *cur, *temp;


	if (!s)
		return;

	for (i = 0; i < SIDTAB_SIZE; i++) {
		last = NULL;
		cur = s->htable[i];
		while (cur != NULL) {
			ret = apply(cur->sid, &cur->context, args);
			if (ret) {
				if (last) {
					last->next = cur->next;
				} else {
					s->htable[i] = cur->next;
				}

				temp = cur;
				cur = cur->next;
				context_destroy(&temp->context);
				free(temp);
			} else {
				last = cur;
				cur = cur->next;
			}
		}
	}

	return;
}


int sidtab_context_to_sid(sidtab_t * s,
			  context_struct_t * context,
			  security_id_t * out_sid)
{
	security_id_t sid;
	sidtab_node_t *cur;
	int i, ret;


	*out_sid = SECSID_NULL;

	/* check to see if there is already a sid for this context */
	for (i = 0; i < SIDTAB_SIZE; i++) {
		cur = s->htable[i];
		while (cur != NULL) {
			if (context_cmp(&cur->context, context)) {
				*out_sid = cur->sid;
				return 0;
			}
			cur = cur->next;
		}
	}

	/* no sid exists; need to create a new entry in the sid table */
	sid = s->nel + 1;
	if (sid == 0) {	
		return -1;
	}
	ret = sidtab_insert(s, sid, context);
	if (ret) {
		return -1;
	}
	*out_sid = sid;
	return 0;
}

/* FLASK */
