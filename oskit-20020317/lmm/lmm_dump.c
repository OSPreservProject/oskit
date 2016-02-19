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
 * dump an LMM memory pool and do a thorough sanity check on it.
 */

#ifndef DEBUG
#define DEBUG
#endif

#include <stdio.h>

#include "lmm.h"

void lmm_dump(lmm_t *lmm)
{
	struct lmm_region *reg;

	printf("lmm_dump(lmm=%p)\n", lmm);

	for (reg = lmm->regions; reg; reg = reg->next)
	{
		struct lmm_node *node;
		oskit_size_t free_check;

		printf(" region %08x-%08x size=%08x flags=%08x pri=%d free=%08x\n",
			reg->min, reg->max, reg->max - reg->min,
			reg->flags, reg->pri, reg->free);

		CHECKREGPTR(reg);

		free_check = 0;
		for (node = reg->nodes; node; node = node->next)
		{
			printf("  node %p-%08x size=%08x next=%p\n", 
				node, (oskit_addr_t)node + node->size, node->size, node->next);

			assert(((oskit_addr_t)node & ALIGN_MASK) == 0);
			assert((node->size & ALIGN_MASK) == 0);
			assert(node->size >= sizeof(*node));
			assert((node->next == 0) || (node->next > node));
			assert((oskit_addr_t)node < reg->max);

			free_check += node->size;
		}

		printf(" free_check=%08x\n", free_check);
		assert(reg->free == free_check);
	}

	printf("lmm_dump done\n");
}

