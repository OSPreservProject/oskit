/*
 * Copyright (c) 1996, 1998, 1999, 2001 University of Utah and the Flux Group.
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
 * Memory object. Mostly an interface to the LMM. The malloc implementation
 * in the C library will look for this interface in the registry, and use it
 * to allocate memory. 
 */
#include <oskit/com.h>
#include <oskit/com/mem.h>
#include <oskit/com/services.h>
#include <oskit/lmm.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>			/* mem_lock - BOGUS! */
#include <oskit/machine/phys_lmm.h>

/* A common symbol that is overridden by the kernel library */
lmm_t			malloc_lmm;

/* the COM object */
static struct oskit_mem_ops mem_ops;
static oskit_mem_t	oskit_mem  = { &mem_ops };

/*
 * Always allocate multiples of MIN_INCR bytes.
 */
#define MIN_INCR	(16*1024)

#define MORECORE(size, flags) ({ \
	void *__mem;							  \
	oskit_size_t __size = ((size) + MIN_INCR - 1) & ~(MIN_INCR - 1);  \
									  \
	mem_unlock();							  \
	if ((__mem = oskit_mem_morecore(__size, flags)) == 0) {		  \
		return 0;						  \
        }								  \
	mem_lock();							  \
	lmm_add_free(&malloc_lmm, __mem, __size);			  \
}) 

#if 0
#define DPRINTF(fmt, args... ) printf(__FUNCTION__  ":" fmt , ## args)
#else
#define DPRINTF(fmt, args... )
#endif

static OSKIT_COMDECL 
mem_query(oskit_mem_t *m, const oskit_iid_t *iid, void **out_ihandle)
{ 
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_mem_iid, sizeof(*iid)) == 0) {
                *out_ihandle = m;
                return 0; 
        }
  
        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
mem_addref(oskit_mem_t *m)
{
	/* No reference counting; only one of them */
	return 1;
}

static OSKIT_COMDECL_U
mem_release(oskit_mem_t *m)
{
	/* No reference counting; only one of them */
	return 1;
}

static void * OSKIT_COMCALL
mem_alloc(oskit_mem_t *m, oskit_u32_t size, oskit_u32_t flags)
{
	lmm_flags_t	lmm_flags = 0;
	oskit_u32_t	*chunk;

	DPRINTF("size = %x\n", size);

	if (flags & OSKIT_MEM_ISADMA_MEM)
		lmm_flags |= LMMF_16MB;

	mem_lock();
	if (flags & OSKIT_MEM_AUTO_SIZE) {
		size += sizeof(oskit_u32_t);

		while (!(chunk = lmm_alloc(&malloc_lmm, size, lmm_flags))) {
			MORECORE(size, lmm_flags);
		}

		*chunk++ = size;
	}
	else {
		while (!(chunk = lmm_alloc(&malloc_lmm, size, lmm_flags))) {
			MORECORE(size, lmm_flags);
		}
	}
	mem_unlock();

	return chunk;
}

static void * OSKIT_COMCALL
mem_realloc(oskit_mem_t *m, void *ptr,
	    oskit_u32_t oldsize, oskit_u32_t newsize, oskit_u32_t flags)
{
	lmm_flags_t	lmm_flags = 0;
	oskit_u32_t	*chunk;

	if (flags & OSKIT_MEM_ISADMA_MEM)
		lmm_flags |= LMMF_16MB;

	mem_lock();
	if (flags & OSKIT_MEM_AUTO_SIZE) {
		oskit_size_t	*op;

		op = (oskit_size_t *)ptr;
		oldsize  = *--op;
		newsize += sizeof(oskit_u32_t);

		while (!(chunk = lmm_alloc(&malloc_lmm, newsize, lmm_flags))) {
			MORECORE(newsize, lmm_flags);
		}

		DPRINTF("0: %p %p %d %d\n", ptr, chunk, oldsize, newsize);

		memcpy(chunk, op, oldsize < newsize ? oldsize : newsize);
		lmm_free(&malloc_lmm, op, oldsize);

		*chunk++ = newsize;
	}
	else {
		while (!(chunk = lmm_alloc(&malloc_lmm, newsize, lmm_flags))) {
			MORECORE(newsize, lmm_flags);
		}
		
		DPRINTF("1: %p %p %d %d\n", ptr, chunk, oldsize, newsize);

		memcpy(chunk, ptr, oldsize < newsize ? oldsize : newsize);
		lmm_free(&malloc_lmm, ptr, oldsize);
	}
	mem_unlock();

	return chunk;
}

static void *OSKIT_COMCALL
mem_alloc_aligned(oskit_mem_t *m,
		  oskit_u32_t size, oskit_u32_t flags, oskit_u32_t align)
{
	lmm_flags_t	lmm_flags = 0;
	oskit_u32_t	*chunk;
	unsigned	shift;

	/* Find the alignment shift in bits.  XXX use proc_ops.h  */
	for (shift = 0; (1 << shift) < align; shift++);

	if (flags & OSKIT_MEM_ISADMA_MEM)
		lmm_flags |= LMMF_16MB;

	mem_lock();
	if (flags & OSKIT_MEM_AUTO_SIZE) {
		size += sizeof(oskit_u32_t);

		while (!(chunk =
			 lmm_alloc_aligned(&malloc_lmm, size, lmm_flags, shift,
					(1 << shift) - sizeof(oskit_u32_t)))) {
			MORECORE(size * 2, lmm_flags);
		}
		
		DPRINTF("0: %p %d %d\n", chunk, size, align);
		
		*chunk++ = size;
	}
	else {
		while (!(chunk = lmm_alloc_aligned(&malloc_lmm,
						size, lmm_flags, shift, 0))) {
			MORECORE(size * 2, lmm_flags);
		}

		DPRINTF("1: %p %d %d\n", chunk, size, align);
	}
	mem_unlock();

	return chunk;
}

static void OSKIT_COMCALL
mem_free(oskit_mem_t *m, void *ptr, oskit_u32_t size, oskit_u32_t flags)
{
	/* Posix says free of NULL does nothing */
	if (! ptr)
		return;

	mem_lock();
	if (flags & OSKIT_MEM_AUTO_SIZE) {
		oskit_u32_t *chunk = (oskit_u32_t *)ptr - 1;

		DPRINTF("0: %p %d 0x%x\n", ptr, *chunk, flags);
		
		lmm_free(&malloc_lmm, chunk, *chunk);
	}
	else {
		DPRINTF("1: %p %d 0x%x\n", ptr, size, flags);

		lmm_free(&malloc_lmm, ptr, size);
	}
	mem_unlock();
}

static oskit_u32_t OSKIT_COMCALL
mem_getsize(oskit_mem_t *m, void *ptr)
{
	if (ptr) {
		oskit_u32_t *chunk = (oskit_u32_t *)ptr - 1;

		return *chunk;
	}
	return 0;
}

static void *OSKIT_COMCALL
mem_alloc_gen(oskit_mem_t *m,
	      oskit_u32_t size, oskit_u32_t flags,
	      oskit_u32_t align_bits, oskit_u32_t align_ofs)
{
	lmm_flags_t	lmm_flags = 0;
	oskit_u32_t	*chunk;

	if (flags & OSKIT_MEM_ISADMA_MEM)
		lmm_flags |= LMMF_16MB;

	/*
	 * Not really all that general. For now, support only non
	 * autosize allocations. 
	 */
	if (flags & OSKIT_MEM_AUTO_SIZE)
		return NULL;

	mem_lock();
	while (!(chunk = lmm_alloc_gen(&malloc_lmm, size, lmm_flags,
				       align_bits, align_ofs, 0, -1))) {
		MORECORE(size * 2, flags);
	}
	DPRINTF("1: %p %d %x %d %d\n", chunk, size, flags, align_bits, align_ofs);
	mem_unlock();

	return chunk;
}

static oskit_size_t OSKIT_COMCALL
mem_avail(oskit_mem_t *m, oskit_u32_t flags)
{
	lmm_flags_t	lmm_flags = 0;
	oskit_size_t	avail;

	if (flags & OSKIT_MEM_ISADMA_MEM)
		lmm_flags |= LMMF_16MB;

	if (flags & OSKIT_MEM_X861MB_MEM)
		lmm_flags |= LMMF_1MB;

	mem_lock();
	avail = lmm_avail(&malloc_lmm, lmm_flags);
	mem_unlock();

	return avail;
}

static OSKIT_COMDECL_V
mem_dump(oskit_mem_t *m)
{
	mem_lock();
	lmm_dump(&malloc_lmm);
	mem_unlock();
}

static struct oskit_mem_ops mem_ops = {
	mem_query,
	mem_addref,
	mem_release,
	mem_alloc,
	mem_realloc,
	mem_alloc_aligned,
	mem_free,
	mem_getsize,
	mem_alloc_gen,
	mem_avail,
	mem_dump,
};

#ifdef KNIT
oskit_mem_t	*oskit_mem_object = &oskit_mem;
#else
/*
 * Not a lot to do in this impl.
 */
oskit_mem_t *
oskit_mem_init(void)
{
	return &oskit_mem;
}
#endif
