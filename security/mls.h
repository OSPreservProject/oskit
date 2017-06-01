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
 * Multi-level security (MLS) policy operations.
 */

#ifndef _MLS_H_
#define _MLS_H_

#include "context.h"
#include "policydb.h"
#include "services.h"

void mls_compute_av(context_struct_t * scontext,
		    context_struct_t * tcontext,
		    class_datum_t * tclass,
		    access_vector_t * allowed);

int mls_compute_context_len(context_struct_t * context);

int mls_sid_to_context(context_struct_t * context,
		       char **scontext);

int mls_context_isvalid(policydb_t *p, context_struct_t * c);

int mls_context_to_sid(char **scontext,
		       context_struct_t * context);

int mls_member_sid(context_struct_t * newcontext,
		   context_struct_t * scontext);

int mls_convert_context(policydb_t * oldp,
			policydb_t * newp,
			context_struct_t * context);

mls_level_t *mls_read_level(FILE * fp);

int mls_read_range(mls_range_t * r,
		   FILE * fp);

int mls_read_perms(mls_perms_t * p,
		   FILE * fp);

#ifndef __KERNEL__
int mls_write_level(mls_level_t * l,
		    FILE * fp);

int mls_write_range(mls_range_t * r,
		    FILE * fp);

int mls_write_perms(mls_perms_t * p,
		    FILE * fp);
#endif


#endif
