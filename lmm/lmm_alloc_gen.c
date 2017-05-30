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

void *lmm_alloc_gen(lmm_t *lmm, oskit_size_t size, lmm_flags_t flags,
		    int align_bits, oskit_addr_t align_ofs,
		    oskit_addr_t in_min, oskit_size_t in_size)
{
	oskit_addr_t in_max = in_min + in_size;
	struct lmm_region *reg;

#if 0
	printf("lmm_alloc_gen %08x\n", size);
	lmm_dump(lmm);
#endif

	assert(lmm != 0);
	assert(size > 0);

	for (reg = lmm->regions; reg; reg = reg->next)
	{
		struct lmm_node **nodep, *node;

		CHECKREGPTR(reg);

		/* First trivially reject the entire region if possible.  */
		if ((flags & ~reg->flags) 
		    || (reg->min >= in_max)
		    || (reg->max <= in_min))
			continue;

		for (nodep = &reg->nodes;
		     (node = *nodep) != 0;
		     nodep = &node->next)
		{
			oskit_addr_t addr;
			struct lmm_node *anode;
			int i;

			assert(((oskit_addr_t)node & ALIGN_MASK) == 0);
			assert(((oskit_addr_t)node->size & ALIGN_MASK) == 0);
			assert((node->next == 0) || (node->next > node));
			assert((oskit_addr_t)node < reg->max);

			/* Now make a first-cut trivial elimination check
			   to skip chunks that are _definitely_ too small.  */
			if (node->size < size)
				continue;

			/* Now compute the address at which
			   the allocated chunk would have to start.  */
			addr = (oskit_addr_t)node;
			if (addr < in_min)
				addr = in_min;
			for (i = 0; i < align_bits; i++)
			{
				oskit_addr_t bit = (oskit_addr_t)1 << i;
				if ((addr ^ align_ofs) & bit)
					addr += bit;
			}

			/* See if the block at the adjusted address
			   is still entirely within the node.  */
			if ((addr - (oskit_addr_t)node + size) > node->size)
				continue;

			/* If the block extends past the range constraint,
			   then all of the rest of the nodes in this region
			   will extend past it too, so stop here. */
			if (addr + size > in_max)
				break;

			/* OK, we can allocate the block from this node.  */

			/* If the allocation leaves at least ALIGN_SIZE
			   space before it, then split the node.  */
			anode = (struct lmm_node*)(addr & ~ALIGN_MASK);
			assert(anode >= node);
			if (anode > node)
			{
				oskit_size_t split_size = (oskit_addr_t)anode
							- (oskit_addr_t)node;
				assert((split_size & ALIGN_MASK) == 0);
				anode->next = node->next;
				anode->size = node->size - split_size;
				node->size = split_size;
				nodep = &node->next;
			}

			/* Now use the first part of the anode
			   to satisfy the allocation,
			   splitting off the tail end if necessary.  */
			size = ((addr & ALIGN_MASK) + size + ALIGN_MASK)
				& ~ALIGN_MASK;
			if (anode->size > size)
			{
				struct lmm_node *newnode;

				/* Split the node and return its head.  */
				newnode = (struct lmm_node*)
						((void*)anode + size);
				newnode->next = anode->next;
				newnode->size = anode->size - size;
				*nodep = newnode;
			}
			else
			{
				/* Remove and return the entire node.  */
				*nodep = anode->next;
			}

			/* Adjust the region's free memory counter.  */
			assert(reg->free >= size);
			reg->free -= size;

#if 0
			printf("lmm_alloc_gen returning %08x\n", addr);
			lmm_dump(lmm);
#endif

			return (void*)addr;
		}
	}

#if 0
	printf("lmm_alloc_gen failed\n");
#endif

	return 0;
}

