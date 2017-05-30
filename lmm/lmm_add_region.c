/*
 * Copyright (c) 1995, 1998 University of Utah and the Flux Group.
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

#include "lmm.h"

void lmm_add_region(lmm_t *lmm, lmm_region_t *reg,
		    void *addr, oskit_size_t size,
		    lmm_flags_t flags, lmm_pri_t pri)
{
	oskit_addr_t min = (oskit_addr_t)addr;
	oskit_addr_t max = min + size;
	struct lmm_region **rp, *r;

	/* Align the start and end addresses appropriately.  */
	min = (min + ALIGN_MASK) & ~ALIGN_MASK;
	max &= ~ALIGN_MASK;

	/* If there's not enough memory to do anything with,
	   then just drop the region on the floor.
	   Since we haven't put it on the lmm's list,
	   we'll never see it again.  */
	if (max <= min)
		return;

	/* Initialize the new region header.  */
	reg->nodes = 0;
	reg->min = min;
	reg->max = max;
	reg->flags = flags;
	reg->pri = pri;
	reg->free = 0;

	/* Add the region to the lmm's region list in descending priority order.
	   For regions with the same priority, sort from largest to smallest
	   to reduce the average amount of list traversing we need to do.  */
	for (rp = &lmm->regions;
	     (r = *rp) && ((r->pri > pri) ||
	     		   ((r->pri == pri) &&
			    (r->max - r->min > reg->max - reg->min)));
	     rp = &r->next)
	{
		assert(r != reg);
		assert((reg->max <= r->min) || (reg->min >= r->max));
	}
	reg->next = r;
	*rp = reg;
}

