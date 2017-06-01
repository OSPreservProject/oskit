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

#include <oskit/com/mem.h>
#include <oskit/lmm.h>
#include <stdlib.h>
#include "native.h"


static struct lmm_region allmem;

/**
 * Setup the malloc_lmm for OSKit/UNIX.  Called from _start().
 */
void
oskitunix_mem_init(void)
{
	extern struct lmm malloc_lmm;
	lmm_add_region(&malloc_lmm, &allmem,
		       0x0, 0xffffffff,
		       /*flags?*/ 0, 
		       /*priority?*/ 0);
}

/*
 * Override the default clientos morecore implementation.
 *
 * Morecore takes OSKIT_MEM_* flags.
 * We assert that the flags are zero because we don't know how
 * to handle them on Unix (and it doesn't seem that we need to know).
 */
void *
oskit_mem_morecore(oskit_size_t size, int flags)
{
	void* more;
	const int MINALLOC = 8*1024;

#ifdef DEBUG	
	extern struct lmm malloc_lmm;
	assert(malloc_lmm.regions != NULL);
#endif

	assert(flags == 0x0);

	if (size < MINALLOC)
		size = MINALLOC;

	more = NATIVEOS(sbrk)(size);

	return more;
}
