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
 * Standard C malloc functions built on top of lmm.
 * Keeps track of the size of the object for you.
 *
 * Can be combined with various lmm adaptors to get thread-safety,
 * caching, etc.
 */
#include "mallocf.h"

#include <string.h>

static inline unsigned int
min(unsigned int x, unsigned int y)
{
        return x < y ? x : y;
}

void *malloc(size_t size)
{
        return malloc_flags(size,0);
}

void *memalign(size_t alignment, size_t size)
{
        return memalign_flags(alignment,size,0);
}

void free(void *chunk)
{
        return free_flags(chunk);
}

void *realloc(void *chunk, oskit_size_t new_size)
{
        oskit_size_t old_size;
        oskit_size_t *new_chunk;

	new_chunk = malloc(new_size);
        if (new_chunk == 0) 
                return 0;

	if (chunk) {
                old_size = *((oskit_size_t*)chunk);
                memcpy(new_chunk, chunk, min(old_size, new_size));
                free(chunk);
        }
        return new_chunk;
}

