/*
 * Copyright (c) 1995, 1998-1999 University of Utah and the Flux Group.
 * All rights reserved.
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

#include <assert.h>

#include <oskit/lmm.h>

struct lmm_node
{
	struct lmm_node *next;
	oskit_size_t size;
};

#define ALIGN_SIZE	sizeof(struct lmm_node)
#define ALIGN_MASK	(ALIGN_SIZE - 1)

#define CHECKREGPTR(p)	{ \
	assert((reg->nodes == 0 && reg->free == 0) \
	       || (oskit_addr_t)reg->nodes >= reg->min); \
	assert(reg->nodes == 0 || (oskit_addr_t)reg->nodes < reg->max); \
	assert(reg->free >= 0); \
	assert(reg->free <= reg->max - reg->min); \
}
