/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Alternate version of memalign
 * that expects the caller to keep track of the size of allocated chunks.
 * This version is _much_ more memory-efficient
 * when allocating many chunks naturally aligned to their (natural) size
 * (e.g. allocating naturally-aligned pages or superpages),
 * because normal memalign requires a prefix between each chunk
 * which will create horrendous fragmentation and memory loss.
 * Chunks allocated with this function must be freed with sfree().
 */

#include "malloc.h"

void *smemalign(size_t alignment, size_t size)
{
	if (!check_memory_object()) return NULL;

	return oskit_mem_alloc_aligned(libc_memory_object, size, 0, alignment);
}
