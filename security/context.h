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
 * A security context is a set of security attributes
 * associated with each subject and object controlled
 * by the security policy.  Security contexts are
 * externally represented as variable-length strings
 * that can be interpreted by a user or application
 * with an understanding of the security policy. 
 * Internally, the security server uses a simple
 * structure.  This structure is private to the
 * security server and can be changed without affecting
 * clients of the security server.
 */

#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "ebitmap.h"

#ifdef CONFIG_FLASK_MLS
#include "mls_types.h"
#endif

/*
 * A security context consists of an authenticated user
 * identity, a role, a type and a MLS range.
 */
typedef struct context_struct {
	__u32 user;
	__u32 role;
	__u32 type;
#ifdef CONFIG_FLASK_MLS
	mls_range_t range;
#endif
} context_struct_t;


#ifdef CONFIG_FLASK_MLS

#define mls_context_init(c) memset(c, 0, sizeof(mls_range_t))

extern int mls_context_cpy(context_struct_t * dst, context_struct_t * src);

#define mls_context_cmp(c1,c2) \
(((c1)->range.level[0].sens == (c2)->range.level[0].sens) && \
 ebitmap_cmp(&(c1)->range.level[0].cat,&(c2)->range.level[0].cat) && \
 ((c1)->range.level[1].sens == (c2)->range.level[1].sens) && \
 ebitmap_cmp(&(c1)->range.level[1].cat,&(c2)->range.level[1].cat))

#define mls_context_destroy(c) \
do { \
	ebitmap_destroy(&(c)->range.level[0].cat); \
	ebitmap_destroy(&(c)->range.level[1].cat); \
        memset(c, 0, sizeof(mls_range_t)); \
} while (0)

#else

#define mls_context_init(c)
#define mls_context_cpy(dst,src) 0
#define mls_context_destroy(c)
#define mls_context_cmp(c1,c2) 1

#endif


#define context_init(c) memset(c, 0, sizeof(context_struct_t))

extern int context_cpy(context_struct_t * dst, context_struct_t * src);

#define context_destroy(c) \
do { \
	(c)->user = 0; \
	(c)->role = 0; \
	(c)->type = 0; \
	mls_context_destroy(c); \
} while (0)

#define context_cmp(c1,c2) \
	(((c1)->user == (c2)->user) && \
	((c1)->role == (c2)->role) && \
	((c1)->type == (c2)->type) && \
	mls_context_cmp(c1, c2))

#endif	/* _CONTEXT_H_ */

/* FLASK */
