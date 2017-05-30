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
 * An object-free interface to the LMM. 
 * Typically used by both malloc in the C library and osenv_mem.
 * Typically combined with other units to add caching, locking and other
 * convenient functionality.
 */
#include <oskit/lmm.h>
#include <knit/c/lmm.h>
#include <oskit/machine/phys_lmm.h>

extern lmm_t malloc_lmm;

void out_add_region(lmm_region_t *lmm_region,
                    void *addr, oskit_size_t size,
                    lmm_flags_t flags, lmm_pri_t pri)
{
        lmm_add_region(&malloc_lmm,lmm_region,addr,size,flags,pri);
}

void out_add_free(void *block, oskit_size_t size)
{
        lmm_add_free(&malloc_lmm,block,size);
}

void out_remove_free(void *block, oskit_size_t size)
{
        lmm_remove_free(&malloc_lmm,block,size);
}

void *out_alloc(oskit_size_t size, lmm_flags_t flags)
{
        return lmm_alloc(&malloc_lmm,size,flags);
}

void *out_alloc_aligned(oskit_size_t size, lmm_flags_t flags,
                        int align_bits, oskit_addr_t align_ofs)
{
        if (align_bits) {
                return lmm_alloc_aligned(&malloc_lmm,size,flags,align_bits,align_ofs);
        } else {
                return lmm_alloc(&malloc_lmm,size,flags);
        }
}

void *out_alloc_page(lmm_flags_t flags)
{
        return lmm_alloc_page(&malloc_lmm,flags);
}

void *out_alloc_gen(oskit_size_t size, lmm_flags_t flags,
                    int align_bits, oskit_addr_t align_ofs,
                    oskit_addr_t bounds_min, oskit_addr_t bounds_max)
{
        return lmm_alloc_gen(&malloc_lmm,size,flags,align_bits,align_ofs,
                             bounds_min,bounds_max);
}

oskit_size_t out_avail(lmm_flags_t flags)
{
        return lmm_avail(&malloc_lmm,flags);
}

void out_find_free(oskit_addr_t *inout_addr,
               oskit_size_t *out_size, lmm_flags_t *out_flags)
{
        lmm_find_free(&malloc_lmm,inout_addr,out_size,out_flags);
}

void out_free(void *block, oskit_size_t size)
{
        lmm_free(&malloc_lmm,block,size);
}

void out_free_page(void *block)
{
        lmm_free_page(&malloc_lmm,block);
}

/* Only available if debugging turned on.  */
void out_dump(void)
{
        lmm_dump(&malloc_lmm);
}

void out_stats(void)
{
        lmm_stats(&malloc_lmm);
}


