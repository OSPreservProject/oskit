/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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
 * Debugging wrapper for memory object.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv_mem.h>

struct mem {
	oskit_osenv_mem_t memi;
	oskit_u32_t count;

	oskit_size_t outstanding;

	/*
	 * This is where the allocations really happen.
	 */
	oskit_osenv_mem_t *mem;
};

static OSKIT_COMDECL
mem_query(oskit_osenv_mem_t *s, const oskit_iid_t *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_osenv_mem_iid, sizeof(*iid)) == 0) {
                *out_ihandle = s;
                return 0;
        }

        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
mem_addref(oskit_osenv_mem_t *m0)
{
        struct mem *m = (struct mem *)m0;
	osenv_assert(m->count);

	return ++m->count;
}

static OSKIT_COMDECL_U
mem_release(oskit_osenv_mem_t *m0)
{
        struct mem *m = (struct mem *)m0;
	oskit_osenv_mem_t *mem;
	osenv_assert(m->count);

	if(--m->count)
		return m->count;

	mem = m->mem;
	oskit_osenv_mem_free(mem, m, 0, sizeof *m);
	osenv_log(OSENV_LOG_INFO, __FUNCTION__": %d outstanding bytes\n",
		  m->outstanding);
	oskit_osenv_mem_release(mem);
	return 0;
}

static void * OSKIT_COMCALL
mem_alloc(oskit_osenv_mem_t *m0, oskit_size_t size,
	  osenv_memflags_t flags, unsigned align)
{
        struct mem *m = (struct mem *)m0;
	void* block;
	osenv_assert(m->count);

	block = oskit_osenv_mem_alloc(m->mem, size, flags, align);
	if (block) {
		int i;
		for(i = 0; i < size; ++i) {
			((char *)block)[i] = 'A';
		}
		m->outstanding += size;
	}
	return block;
}

static OSKIT_COMDECL_V
mem_free(oskit_osenv_mem_t *m0, void *block,
	 osenv_memflags_t flags, oskit_size_t size)
{
        struct mem *m = (struct mem *)m0;
	osenv_assert(m->count);

	{
		int i;
		for(i = 0; i < size; ++i) {
			((char *)block)[i] = 'X';
		}

		m->outstanding -= size;
	}
	oskit_osenv_mem_free(m->mem, block, flags, size);
}

static oskit_addr_t OSKIT_COMCALL
mem_getphys(oskit_osenv_mem_t *m0, oskit_addr_t va)
{
        struct mem *m = (struct mem *)m0;
	osenv_assert(m->count);

	return oskit_osenv_mem_getphys(m->mem, va);
}

static oskit_addr_t OSKIT_COMCALL
mem_getvirt(oskit_osenv_mem_t *m0, oskit_addr_t pa)
{
        struct mem *m = (struct mem *)m0;
	osenv_assert(m->count);

	return oskit_osenv_mem_getvirt(m->mem, pa);
}

static oskit_addr_t OSKIT_COMCALL
mem_physmax(oskit_osenv_mem_t *m0)
{
        struct mem *m = (struct mem *)m0;
	osenv_assert(m->count);

	return oskit_osenv_mem_physmax(m->mem);
}

static OSKIT_COMDECL_U
mem_mapphys(oskit_osenv_mem_t *m0, oskit_addr_t pa,
	    oskit_size_t size, void **addr, int flags)
{
        struct mem *m = (struct mem *)m0;
	osenv_assert(m->count);

	return oskit_osenv_mem_mapphys(m->mem, pa, size, addr, flags);
}

static struct oskit_osenv_mem_ops osenv_mem_ops = {
	mem_query,
	mem_addref,
	mem_release,
	mem_alloc,
	mem_free,
	mem_getphys,
	mem_getvirt,
	mem_physmax,
	mem_mapphys,
};

/*
 * Return a reference to the one and only memory interface object.
 */
oskit_osenv_mem_t *
oskit_create_osenv_memdebug(oskit_osenv_mem_t *mem)
{
	struct mem *m;
	m = oskit_osenv_mem_alloc(mem, sizeof *m, OSENV_NONBLOCKING, 0);
	if (!m)
		return 0;

	m->memi.ops    = &osenv_mem_ops;
	m->count       = 1;
	m->mem         = mem;
	m->outstanding = 0;

	return &m->memi;
}
