/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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

void lmm_remove_free(lmm_t *lmm, void *block, oskit_size_t block_size)
{
	oskit_addr_t rstart = (oskit_addr_t)block;
	oskit_addr_t rend = rstart + block_size;
	assert(rend > rstart);

	while (rstart < rend)
	{
		oskit_addr_t size;
		lmm_flags_t flags;
		void *ptr;

		lmm_find_free(lmm, &rstart, &size, &flags);
		assert(rstart >= (oskit_addr_t)block);
		if ((size == 0) || (rstart >= rend))
			break;
		if (rstart + size > rend)
			size = rend - rstart;
		ptr = lmm_alloc_gen(lmm, size, flags, 0, 0,
				    rstart, size);
		assert((oskit_addr_t)ptr == rstart);
	}
}

