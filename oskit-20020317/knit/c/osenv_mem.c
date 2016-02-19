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
 * Memdebug wrapper
 */
#include <oskit/c/malloc.h>
#include <oskit/c/stdlib.h>
#include <oskit/dev/dev.h>

#include <oskit/debug.h>

extern oskit_u32_t size; // size of guard blocks in words

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

#define GUARD_SIZE(x) ((x) * sizeof(oskit_u32_t))

inline void
set_guard(void *m)
{
        int i;
        for(i=0; i<size; ++i) {
                ((oskit_u32_t*)m)[i] = 0xdeadbeef;
        }
}

inline void
check_guard(char * which, oskit_u32_t *m)
{
        int i;
        for(i=0; i<size; ++i) {
                oskit_u32_t x = ((oskit_u32_t*)m)[i];
                if (x != 0xdeadbeef) {
                        panic("guard corrupted: %s[%d] == %x\n",which,i,x);
                }
        }
}

inline void
scrub(char *block, oskit_size_t size)
{
        int i;
        for(i=0; i<size; ++i) {
                block[i] = 0xff;
        }
}

// ToDo: doesn't respect alignment
void *
osenv_mem_alloc(oskit_size_t sz, osenv_memflags_t flags, unsigned align)
{
        oskit_size_t realsize = sz + 2 * GUARD_SIZE(size);
        void *block = in_osenv_mem_alloc(realsize,flags,align);
        set_guard(block);
        set_guard(block+GUARD_SIZE(size)+sz);
        return block+GUARD_SIZE(size);
}

void
osenv_mem_free(void *block, osenv_memflags_t flags, oskit_size_t sz)
{
        oskit_size_t realsize = sz + 2 * GUARD_SIZE(size);
        block -= GUARD_SIZE(size);
        check_guard("pre", block);
        // Sigh, we can't check the end unless we know the size
        if (! (flags & OSENV_AUTO_SIZE)) {
                check_guard("post", block+GUARD_SIZE(size)+sz);
                scrub(block,realsize);
        }
        in_osenv_mem_free(block,flags,realsize);
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
