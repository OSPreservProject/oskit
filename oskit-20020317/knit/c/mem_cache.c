/*
 * Copyright (c) 1996, 1998-2000 University of Utah and the Flux Group.
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
 * A simple cache wrapper fro osenv_mem
 */
#include <oskit/c/malloc.h>
#include <oskit/c/stdlib.h>
#include <oskit/dev/dev.h>

#include <oskit/debug.h>

extern oskit_u32_t size;    // size to cache

#define ENTRIES 30

void* cache[ENTRIES];
int index = 0;        // first free entry

extern void *
in_osenv_mem_alloc(oskit_size_t size, osenv_memflags_t flags, unsigned align);

extern void
in_osenv_mem_free(void *block, osenv_memflags_t flags, oskit_size_t size);

extern int
in_osenv_mem_map_phys(oskit_addr_t pa, oskit_size_t size, void **addr, int flags);

extern oskit_addr_t
in_osenv_mem_phys_max(void);

extern oskit_addr_t
in_osenv_mem_get_phys(oskit_addr_t addr);

extern oskit_addr_t
in_osenv_mem_get_virt(oskit_addr_t addr);

// ToDo: ignores alignment
void *
osenv_mem_alloc(oskit_size_t sz, osenv_memflags_t flags, unsigned align)
{
        if (sz == size && index > 0) {
                return cache[--index];
        } else {
                return in_osenv_mem_alloc(sz,flags,align);
        }
}

void
osenv_mem_free(void *block, osenv_memflags_t flags, oskit_size_t sz)
{
        if (! (flags & OSENV_AUTO_SIZE) && sz == size && index < ENTRIES) {
                cache[index++] = block;
        } else {
                in_osenv_mem_free(block,flags,sz);
        }
}

int
osenv_mem_map_phys(oskit_addr_t pa, oskit_size_t size, void **addr, int flags)
{
        return in_osenv_mem_map_phys(pa,size,addr,flags);
}


oskit_addr_t
osenv_mem_phys_max(void)
{
        return in_osenv_mem_phys_max();
}

oskit_addr_t
osenv_mem_get_phys(oskit_addr_t addr)
{
        return in_osenv_mem_get_phys(addr);
}

oskit_addr_t
osenv_mem_get_virt(oskit_addr_t addr)
{
        return in_osenv_mem_get_virt(addr);
}
