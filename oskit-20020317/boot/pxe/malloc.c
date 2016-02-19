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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <oskit/lmm.h>
#include <oskit/machine/phys_lmm.h>

/*
 * Avoid dragging in libc. Not enough space!
 */
char *
local_malloc(int size)
{
	oskit_u32_t	*chunk;
	
	chunk = lmm_alloc(&malloc_lmm, size, 0);
	if (!chunk)
		panic(__FUNCTION__ ": lmm_alloc failed for %s bytes", size);

	memset(chunk, 0, size);

	return (char *) chunk;
}

void *
malloc(size_t size)
{
	return local_malloc(size);
}

void *
mustmalloc(size_t size)
{
	void *buf;

	buf = malloc(size);

	return buf;
}

void *
mustcalloc(size_t nelt, size_t eltsize)
{
	void *buf;

	buf = malloc(nelt * eltsize);

	return buf;
}
