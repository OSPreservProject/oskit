/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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
 * Standard C malloc functions built on top of lmm but exposing the lmm 
 * flags.
 * Keeps track of the size of the object for you.
 *
 * Can be combined with various lmm adaptors to get thread-safety,
 * caching, etc.
 */
#include <oskit/c/malloc.h>

#include <oskit/lmm.h>
#include <knit/c/lmm.h>
#include <string.h>

#include "bitops.h"

void *malloc_flags(size_t size, lmm_flags_t flags)
{
        oskit_size_t *chunk;
	chunk = in_alloc(size + sizeof(oskit_size_t), flags);
        if (chunk) 
                *chunk++ = size;
        return chunk;
}

void *memalign_flags(size_t alignment, size_t size, lmm_flags_t flags)
{
        unsigned int shift = log2(alignment);
        oskit_size_t *chunk;
	chunk = in_alloc_aligned(size, flags, shift, (1 << shift) - sizeof(oskit_size_t));
        if (chunk) 
                *chunk++ = size;
        return chunk;
}

void free_flags(void *chunk)
{
        oskit_size_t size;
	/* Posix says free of NULL does nothing */
	if (! chunk)
		return;
        (oskit_size_t*)chunk -= 1;
	size = *((oskit_size_t*)chunk);
	in_free(chunk, size + sizeof(oskit_size_t));
}
