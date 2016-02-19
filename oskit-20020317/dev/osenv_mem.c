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
 * Default memory interface object.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv_mem.h>

/*
 * There is one and only one memory interface in this implementation.
 */
static struct oskit_osenv_mem_ops	osenv_mem_ops;
#ifdef KNIT
       struct oskit_osenv_mem		osenv_mem_object = {&osenv_mem_ops};
#else
static struct oskit_osenv_mem		osenv_mem_object = {&osenv_mem_ops};
#endif

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
mem_addref(oskit_osenv_mem_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_U
mem_release(oskit_osenv_mem_t *s)
{
	/* Only one object */
	return 1;
}

static void * OSKIT_COMCALL
mem_alloc(oskit_osenv_mem_t *o, oskit_size_t size,
	  osenv_memflags_t flags, unsigned align)
{
	return osenv_mem_alloc(size, flags, align);
}

static OSKIT_COMDECL_V
mem_free(oskit_osenv_mem_t *o, void *block,
	 osenv_memflags_t flags, oskit_size_t size)
{
	osenv_mem_free(block, flags, size);
}

static oskit_addr_t OSKIT_COMCALL
mem_getphys(oskit_osenv_mem_t *o, oskit_addr_t va)
{
	return osenv_mem_get_phys(va);
}

static oskit_addr_t OSKIT_COMCALL
mem_getvirt(oskit_osenv_mem_t *o, oskit_addr_t pa)
{
	return osenv_mem_get_virt(pa);
}

static oskit_addr_t OSKIT_COMCALL
mem_physmax(oskit_osenv_mem_t *o)
{
	return osenv_mem_phys_max();
}

static OSKIT_COMDECL_U
mem_mapphys(oskit_osenv_mem_t *o, oskit_addr_t pa,
	    oskit_size_t size, void **addr, int flags)
{
	return osenv_mem_map_phys(pa, size, addr, flags);
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

#ifndef KNIT
/*
 * Return a reference to the one and only memory interface object.
 */
oskit_osenv_mem_t *
oskit_create_osenv_mem(void)
{
	return &osenv_mem_object;
}
#endif
