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

void lmm_find_free(lmm_t *lmm, oskit_addr_t *inout_addr,
		   oskit_size_t *out_size, lmm_flags_t *out_flags)
{
	struct lmm_region *reg;
	oskit_addr_t start_addr = (*inout_addr + ALIGN_MASK) & ~ALIGN_MASK;
	oskit_addr_t lowest_addr = (oskit_addr_t)-1;
	oskit_size_t lowest_size = 0;
	unsigned lowest_flags = 0;

	for (reg = lmm->regions; reg; reg = reg->next)
	{
		struct lmm_node *node;

		if ((reg->nodes == 0)
		    || (reg->max <= start_addr)
		    || (reg->min > lowest_addr))
			continue;

		for (node = reg->nodes; node; node = node->next)
		{
			assert((oskit_addr_t)node >= reg->min);
			assert((oskit_addr_t)node < reg->max);

			if ((oskit_addr_t)node >= lowest_addr)
				break;
			if ((oskit_addr_t)node + node->size > start_addr)
			{
				if ((oskit_addr_t)node > start_addr)
				{
					lowest_addr = (oskit_addr_t)node;
					lowest_size = node->size;
				}
				else
				{
					lowest_addr = start_addr;
					lowest_size = node->size
						- (lowest_addr - (oskit_addr_t)node);
				}
				lowest_flags = reg->flags;
				break;
			}
		}
	}

	*inout_addr = lowest_addr;
	*out_size = lowest_size;
	*out_flags = lowest_flags;
}

