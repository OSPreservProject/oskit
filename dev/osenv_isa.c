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
 * Default isa goo object.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <oskit/error.h>
#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/device.h>
#include <oskit/dev/native.h>
#include <oskit/dev/osenv_isa.h>

/*
 * There is one and only one pci config interface in this implementation.
 */
static struct oskit_osenv_isa_ops	osenv_isa_ops;
#ifdef KNIT
       struct oskit_osenv_isa		osenv_isa_object = {&osenv_isa_ops};
#else
static struct oskit_osenv_isa		osenv_isa_object = {&osenv_isa_ops};
#endif

static OSKIT_COMDECL
isa_query(oskit_osenv_isa_t *s, const oskit_iid_t *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_osenv_isa_iid, sizeof(*iid)) == 0) {
                *out_ihandle = s;
                return 0;
        }

        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
isa_addref(oskit_osenv_isa_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_U
isa_release(oskit_osenv_isa_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL
isa_bus_init(oskit_osenv_isa_t *o)
{
	return osenv_isabus_init();
}

static struct oskit_isabus *OSKIT_COMCALL
isa_bus_getbus(oskit_osenv_isa_t *o)
{
	return osenv_isabus_getbus();
}

static OSKIT_COMDECL
isa_bus_addchild(oskit_osenv_isa_t *o, oskit_u32_t addr, oskit_device_t *dev)
{
	return osenv_isabus_addchild(addr, dev);
}

static OSKIT_COMDECL
isa_bus_remchild(oskit_osenv_isa_t *o, oskit_u32_t addr)
{
	return osenv_isabus_remchild(addr);
}

/*
 * DRQ manipulation.
 */
static OSKIT_COMDECL
isa_dma_alloc(oskit_osenv_isa_t *o, int channel)
{
	return osenv_isadma_alloc(channel);
}

static OSKIT_COMDECL_V
isa_dma_free(oskit_osenv_isa_t *o, int channel)
{
	return osenv_isadma_free(channel);
}

static struct oskit_osenv_isa_ops osenv_isa_ops = {
	isa_query,
	isa_addref,
	isa_release,
	isa_bus_init,
	isa_bus_getbus,
	isa_bus_addchild,
	isa_bus_remchild,
	isa_dma_alloc,
	isa_dma_free,
};

#ifndef KNIT
/*
 * Return a reference to the one and only isa goo object.
 */
oskit_osenv_isa_t *
oskit_create_osenv_isa(void)
{
	return &osenv_isa_object;
}
#endif
