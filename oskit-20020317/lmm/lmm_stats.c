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
/*
 * Debugging routine:
 * dump a summary of LMM memory pool usage.
 */

#ifndef DEBUG
#define DEBUG
#endif

#include <stdio.h>

#include "lmm.h"

void lmm_stats(lmm_t *lmm)
{
	struct lmm_region *reg;
	unsigned int regions, nodes, memfree;

	regions = nodes = memfree = 0;
	for (reg = lmm->regions; reg; reg = reg->next)
	{
		struct lmm_node *node;
		oskit_size_t free_check;

		CHECKREGPTR(reg);

		regions++;

		free_check = 0;
		for (node = reg->nodes; node; node = node->next)
		{
			assert(((oskit_addr_t)node & ALIGN_MASK) == 0);
			assert((node->size & ALIGN_MASK) == 0);
			assert(node->size >= sizeof(*node));
			assert((node->next == 0) || (node->next > node));
			assert((oskit_addr_t)node < reg->max);

			nodes++;

			free_check += node->size;
		}
		assert(reg->free == free_check);

		memfree += reg->free;
	}

	printf("LMM=%p: %u bytes in %u regions, %d nodes\n",
	       lmm, memfree, regions, nodes);
}

