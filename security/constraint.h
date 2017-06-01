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
 * A constraint is a condition that must be satisfied in
 * order for one or more permissions to be granted.  
 * Constraints are used to impose additional restrictions
 * beyond the type-based rules specified in the access 
 * vector table (avtab), such as restrictions based on
 * user identity or role.  The most common usage of 
 * constraints is to prevent a process from transitioning 
 * to a new user identity or role unless it is in a privileged
 * domain.  
 */

#ifndef _CONSTRAINT_H_
#define _CONSTRAINT_H_

#include "ebitmap.h"

typedef struct constraint_expr {
#define CONSTRAINT_EXPR_TYPE_NOT		1	/* NOT `left' */
#define CONSTRAINT_EXPR_TYPE_AND		2	/* `left' AND `right' */
#define CONSTRAINT_EXPR_TYPE_OR			3	/* `left' OR `right' */
#define CONSTRAINT_EXPR_TYPE_SAMEUSER		4	/* source user == target user */
#define CONSTRAINT_EXPR_TYPE_TYPE_SOURCE	5	/* source type is in `bitmap' */
#define CONSTRAINT_EXPR_TYPE_TYPE_TARGET	6	/* target type is in `bitmap' */
#define CONSTRAINT_EXPR_TYPE_ROLE_SOURCE	7	/* source role is in `bitmap' */
#define CONSTRAINT_EXPR_TYPE_ROLE_TARGET	8	/* target role is in `bitmap' */
#define CONSTRAINT_EXPR_TYPE_ROLE_RELATION	9	/* relationship between source role and target role is in `relation' */
	__u32 expr_type;	/* expression type */

#define CONSTRAINT_EXPR_VALUE_RELATION_DOM	1	/* source role dominates */
#define CONSTRAINT_EXPR_VALUE_RELATION_DOMBY	2	/* target role dominates */
#define CONSTRAINT_EXPR_VALUE_RELATION_EQ	3	/* equivalent roles */
#define CONSTRAINT_EXPR_VALUE_RELATION_INCOMP	4	/* incomparable roles */
	__u32 relation;	/* relation value */

	ebitmap_t bitmap;	/* types or roles */

	struct constraint_expr *left;
	struct constraint_expr *right;

	__u32 count;		/* reference count */
} constraint_expr_t;


typedef struct constraint_node {
	access_vector_t permissions;	/* constrained permissions */
	constraint_expr_t *expr;	/* constraint on permissions */
	struct constraint_node *next;	/* next constraint */
} constraint_node_t;

#endif	/* _CONSTRAINT_H_ */

/* FLASK */
