/*
 * Copyright (c) 1996-1999 University of Utah and the Flux Group.
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
 * Wrap the memdebug_alloc, varying the parameters based on which
 * allocator is called.
 */

#include "memdebug.h"
#include <string.h>	/* memcpy */

/*
 * Debugging the memdebug library is easier if you don't include
 * smalloc() etc, as they get used in a lot of the
 * startup routines in libsac.
 */
#define WRAP_SMALLOC_ETC 1

/*
 * malloc wrapper
 */
void *
memdebug_traced_malloc(size_t bytes, unsigned flags, char *file, int line)
{
	return memdebug_alloc(0, bytes, flags, MALLOC_INIT, 
			       MALLOC_CALLER, 
			       file, line);
}

void* 
malloc(size_t bytes)
{
 	return memdebug_traced_malloc(bytes, 0,
				      0, 0);
}

/*
 * memalign wrapper
 */
void *
memdebug_traced_memalign(size_t alignment, size_t size, unsigned flags,
			 char *file, int line)
{
	return memdebug_alloc(alignment, size, flags, MALLOC_INIT,
			       MALLOC_CALLER, 
			       file, line);
}

void *
memalign(size_t alignment, size_t size)
{
	return memdebug_traced_memalign(alignment, size, 0,
					0, 0);
}




/*
 * calloc wrapper.
 */
void* 
memdebug_traced_calloc(size_t n, size_t elem_size, char *file, int line)
{
	return memdebug_alloc(0, n * elem_size, 0, 0,
			       MALLOC_CALLER,
			       file, line);
}

void* 
calloc(size_t n, size_t elem_size)
{
	return memdebug_traced_calloc(n, elem_size,
				      0, 0);
}



/*
 * free wrapper
 */
void
memdebug_traced_free(void* mem, char *file, int line)
{
	memdebug_free(mem, FREE_WIPE, 
		      MALLOC_CALLER, -1,
		      file, line);
}

void 
free(void* mem)
{
	memdebug_traced_free(mem, 
			     0, 0);
}



/*
 * realloc wrapper.
 */
void* 
memdebug_traced_realloc(void* oldmem, size_t bytes, char *file, int line)
{
	void *newmem = memdebug_traced_malloc(bytes, 0, file, line);
	if (!newmem) return 0;
	if (oldmem)
	{
		memcpy(newmem, oldmem, bytes);
		memdebug_traced_free(oldmem, file, line);
	}
	return newmem;
}
	
void* 
realloc(void* oldmem, size_t bytes)
{
	return memdebug_traced_realloc(oldmem, bytes, 
				       0, 0);
}

#if WRAP_SMALLOC_ETC
/*
 * smalloc wrapper. 
 */
void*
memdebug_traced_smalloc (size_t size, unsigned flags, char *file, int line)
{
	return memdebug_alloc(0, size, flags, SMALLOC_INIT, 
			       SMALLOC_CALLER,
			       file, line);
}

void* 
smalloc (size_t size)
{
	return memdebug_traced_smalloc(size, 0, 0, 0);
}



/*
 * sfree wrapper
 */
void 
memdebug_traced_sfree(void* mem, size_t chunksize, char *file, int line)
{
	memdebug_free(mem, SFREE_WIPE, 
		      SMALLOC_CALLER, chunksize,
		      file, line);
}

void 
sfree(void* mem, size_t chunksize)
{
	memdebug_traced_sfree(mem, chunksize,
			      0, 0);
}


/*
 * smemalign wrapper
 */
void *
memdebug_traced_smemalign(size_t alignment, size_t size, unsigned flags,
			  char *file, int line)
{
	return memdebug_alloc(alignment, size, flags, SMALLOC_INIT,
			       SMALLOC_CALLER, 
			       file, line);
}

void *
smemalign(size_t alignment, size_t size)
{
	return memdebug_traced_smemalign(alignment, size, 0,
					 0, 0);
}
#endif
