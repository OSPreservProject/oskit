/*
 * Copyright (c) 1995-1996, 1998-1999 University of Utah and the Flux Group.
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

void *lmm_alloc(lmm_t *lmm, oskit_size_t size, lmm_flags_t flags)
{
	struct lmm_region *reg;

	assert(lmm != 0);
	assert(size > 0);

	size = (size + ALIGN_MASK) & ~ALIGN_MASK;

	for (reg = lmm->regions; reg; reg = reg->next)
	{
		struct lmm_node **nodep, *node;

		CHECKREGPTR(reg);

		if (flags & ~reg->flags)
			continue;

		for (nodep = &reg->nodes;
		     (node = *nodep) != 0;
		     nodep = &node->next)
		{
			assert(((oskit_addr_t)node & ALIGN_MASK) == 0);
			assert(((oskit_addr_t)node->size & ALIGN_MASK) == 0);
			assert((node->next == 0) || (node->next > node));
			assert((oskit_addr_t)node < reg->max);

			if (node->size >= size)
			{
				if (node->size > size)
				{
					struct lmm_node *newnode;

					/* Split the node and return its head */
					newnode = (struct lmm_node*)
							((void*)node + size);
					newnode->next = node->next;
					newnode->size = node->size - size;
					*nodep = newnode;
				}
				else
				{
					/* Remove and return the entire node. */
					*nodep = node->next;
				}

				/* Adjust the region's free memory counter.  */
				assert(reg->free >= size);
				reg->free -= size;

				return (void*)node;
			}
		}
	}

	return 0;
}

