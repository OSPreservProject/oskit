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
 * Add locking to the malloc_lmm.
 */
#include <oskit/lmm.h>
#include <oskit/page.h>
#include <knit/c/lmm.h>
#include <oskit/machine/phys_lmm.h>

/*
 * Allocators call this function when there is a memory
 * shortage. It should try to scare up more memory, and return a
 * pointer to that memory so that the memory object can add it to its
 * pool. Returns zero if no more memory can be found.
 */
extern void *oskit_lmm_morecore(oskit_size_t size, lmm_flags_t flags);

/*
 * Always allocate multiples of MIN_INCR bytes.
 */
#define MIN_INCR	(16*1024)

static int
morecore(oskit_size_t size, lmm_flags_t flags)
{
        void *mem;
        size = ((size) + MIN_INCR - 1) & ~(MIN_INCR - 1);
	if ((mem = oskit_lmm_morecore(size, flags)) == 0) {
		return 1;					
        } else {						
                lmm_add_free(&malloc_lmm, mem, size);
                return 0;
        }
}

void out_add_region(lmm_region_t *lmm_region,
                    void *addr, oskit_size_t size,
                    lmm_flags_t flags, lmm_pri_t pri)
{
        in_add_region(lmm_region,addr,size,flags,pri);
}

void out_add_free(void *block, oskit_size_t size)
{
        in_add_free(block,size);
}

void out_remove_free(void *block, oskit_size_t size)
{
        in_remove_free(block,size);
}

void *out_alloc(oskit_size_t size, lmm_flags_t flags)
{
        void* r;
        while ( (r = in_alloc(size,flags))
                || morecore(size,flags)) {
        }                
        return r;
}

void *out_alloc_aligned(oskit_size_t size, lmm_flags_t flags,
                    int align_bits, oskit_addr_t align_ofs)
{
        void* r;
        while ( (r = in_alloc_aligned(size,flags,align_bits,align_ofs))
                || morecore(size*2,flags)) {
        }
        return r;
}

void *out_alloc_page(lmm_flags_t flags)
{
        void* r;
        while ( (r = in_alloc_page(flags))
                || morecore(PAGE_SIZE*2,flags)) {
        }
        return r;
}

void *out_alloc_gen(oskit_size_t size, lmm_flags_t flags,
                int align_bits, oskit_addr_t align_ofs,
                oskit_addr_t bounds_min, oskit_addr_t bounds_max)
{
        void* r;
        while ( (r = in_alloc_gen(size,flags,align_bits,align_ofs,
                                  bounds_min,bounds_max))
                || morecore(size*2,flags)) {
        }
        return r;
}

oskit_size_t out_avail(lmm_flags_t flags)
{
        oskit_size_t r;
        r = in_avail(flags);
        return r;
}

void out_find_free(oskit_addr_t *inout_addr,
                   oskit_size_t *out_size, lmm_flags_t *out_flags)
{
        in_find_free(inout_addr,out_size,out_flags);
}

void out_free(void *block, oskit_size_t size)
{
        in_free(block,size);
}

void out_free_page(void *block)
{
        in_free_page(block);
}

/* Only available if debugging turned on.  */
void out_dump(void)
{
        in_dump();
}

void out_stats(void)
{
        in_stats();
}
