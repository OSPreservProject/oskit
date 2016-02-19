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

void lmm_add_free(lmm_t *lmm, void *block, oskit_size_t size)
{
	struct lmm_region *reg;
	oskit_addr_t min = (oskit_addr_t)block;
	oskit_addr_t max = min + size;

	/* Restrict the min and max further to be properly aligned.
	   Note that this is the opposite of what lmm_free() does,
	   because lmm_free() assumes the block was allocated with lmm_alloc()
	   and thus would be a subset of a larger, already-aligned free block.
	   Here we can assume no such thing.  */
	min = (min + ALIGN_MASK) & ~ALIGN_MASK;
	max &= ~ALIGN_MASK;
	assert(max >= min);

	/* If after alignment we have nothing left, we're done.  */
	if (max == min)
		return;

	/* Add the block to the free list(s) of whatever region(s) it overlaps.
	   If some or all of the block doesn't fall into any existing region,
	   then that memory is simply dropped on the floor.  */
	for (reg = lmm->regions; reg; reg = reg->next)
	{
		assert(reg->min < reg->max);
		assert((reg->min & ALIGN_MASK) == 0);
		assert((reg->max & ALIGN_MASK) == 0);

		if ((max > reg->min) && (min < reg->max))
		{
			oskit_addr_t new_min = min, new_max = max;

			/* Only add the part of the block
			   that actually falls within this region.  */
			if (new_min < reg->min)
				new_min = reg->min;
			if (new_max > reg->max)
				new_max = reg->max;
			assert(new_max > new_min);

			/* Add the block.  */
			lmm_free(lmm, (void*)new_min, new_max - new_min);
		}
	}
}

