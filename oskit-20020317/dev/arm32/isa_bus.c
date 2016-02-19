/*
 * Copyright (c) 1996-1999, 2001 University of Utah and the Flux Group.
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
 * Basic ISA bus "device driver" for x86 PCs.
 * Implements the ISA bus device node and corresponding COM interface.
 */

#include <stdio.h>
#include <oskit/debug.h>
#include <oskit/boolean.h>
#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/device.h>
#include <oskit/dev/bus.h>
#include <oskit/dev/membus.h>
#include <oskit/dev/isa.h>
#include <oskit/dev/native.h>
#include <oskit/arm32/pio.h>
#include <oskit/c/string.h>

/*
 * Since this "driver" only supports one ISA bus in the system,
 * we only need to export one device node object,
 * so everything can be static.
 * Since everything's static, we don't have to do reference counting.
 */

/* List of child devices attached to this bus, in no particular order */
static struct busnode {
	struct busnode *next;
	oskit_u32_t addr;
	oskit_device_t *dev;
} *nodes;


static OSKIT_COMDECL
query(oskit_isabus_t *bus, const oskit_iid_t *iid, void **out_ihandle)
{
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_driver_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_device_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_bus_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_isabus_iid, sizeof(*iid)) == 0) {
		*out_ihandle = bus;
		return 0;
	}
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
addref(oskit_isabus_t *bus)
{
	return 1;
}

static OSKIT_COMDECL_U
release(oskit_isabus_t *bus)
{
	return 1;
}

static OSKIT_COMDECL
getinfo(oskit_isabus_t *bus, oskit_devinfo_t *out_info)
{
	out_info->name = "isa";
	out_info->description = "Industry Standard Architecture (ISA) Bus";
	out_info->vendor = NULL;
	out_info->author = "Flux Project, University of Utah";
	out_info->version = NULL; /* OSKIT_VERSION or somesuch? */
	return 0;
}

static OSKIT_COMDECL
getdriver(oskit_isabus_t *dev, oskit_driver_t **out_driver)
{
	/*
	 * Our driver node happens to be the same COM object
	 * as our device node, making this really easy...
	 */
	*out_driver = (oskit_driver_t*)dev;
	return 0;
}

/*** Bus device registration ***/

static OSKIT_COMDECL
getchildio(oskit_isabus_t *bus, oskit_u32_t idx, oskit_device_t **out_fdev,
		oskit_u32_t *out_addr)
{
	struct busnode *n;

	/* Find the node with this index number */
	for (n = nodes; ; n = n->next) {
		if (n == NULL)
			return OSKIT_E_DEV_NOMORE_CHILDREN;
		if (idx == 0)
			break;
		idx--;
	}

	/* Return a reference to the device attached here */
	*out_fdev = n->dev;
	oskit_device_addref(n->dev);
	*out_addr = n->addr;
	return 0;
}

static OSKIT_COMDECL
getchildaddr(oskit_isabus_t *bus, oskit_u32_t idx, oskit_device_t **out_fdev,
		oskit_addr_t *out_addr)
{
	oskit_error_t err;
	oskit_u32_t addr;

	err = getchildio(bus, idx, out_fdev, &addr);
	if (err)
		return err;

	*out_addr = 0;	/* XX supply a memory address if memory-mapped? */
	return 0;
}

static OSKIT_COMDECL
getchild(oskit_isabus_t *bus, oskit_u32_t idx, oskit_device_t **out_fdev,
	  char *out_pos)
{
	oskit_error_t err;
	oskit_u32_t addr;

	err = getchildio(bus, idx, out_fdev, &addr);
	if (err)
		return err;

	sprintf(out_pos, "0x%04x", (unsigned)addr);
	return 0;
}

static OSKIT_COMDECL
addchild(oskit_isabus_t *bus, oskit_u32_t addr, oskit_device_t *dev)
{
	struct busnode *n;
	struct busnode *tmp;

	/* XXX sanity check params */
	/* XXX check for addr collisions */

	/* Create a new device node */
	n = osenv_mem_alloc(sizeof(*n), OSENV_PHYS_WIRED, 0);
	if (n == NULL)
		return OSKIT_E_OUTOFMEMORY;

	n->next = NULL;
	n->dev = dev; oskit_device_addref(dev);
	n->addr = addr;

	/*
	 * We need to add it to the end of the list so items
	 * appear in the order in which they are probed so
	 * people don't get confused.
	 */
	if (!nodes) {
		nodes = n;
		return 0;
	}
	for (tmp = nodes; tmp->next; tmp=tmp->next) ;
	tmp->next = n;

	return 0;
}

static OSKIT_COMDECL
remchild(oskit_isabus_t *bus, oskit_u32_t addr)
{
	struct busnode **np, *n;

	for (np = &nodes; (n = *np) != NULL; np = &n->next) {
		if (n->addr == addr) {
			*np = n->next;
			oskit_device_release(n->dev);
			osenv_mem_free(n, OSENV_PHYS_WIRED, sizeof(*n));
			return 0;
		}
	}

	return OSKIT_E_DEV_NOSUCH_CHILD;
}


/*** ISA bus probing ***/

static OSKIT_COMDECL
probe(oskit_isabus_t *bus)
{
	oskit_isa_driver_t **drivers;
	unsigned count, i, found;
	oskit_error_t rc;

	/* Find the set of registered drivers for ISA bus devices */
	rc = osenv_driver_lookup(&oskit_isa_driver_iid, (void***)&drivers);
	if (OSKIT_FAILED(rc))
		return rc;
	count = rc;

	/* Call each driver's probe routine in turn, in priority order */
	found = 0;
	for (i = 0; i < count; i++) {
		rc = oskit_isa_driver_probe(drivers[i], bus);
		if (OSKIT_SUCCEEDED(rc))
			found += rc;
		oskit_isa_driver_release(drivers[i]);
	}

	/* Note that osenv_driver_lookup uses malloc, not osenv_mem_alloc */
	free(drivers);

	return found;
}


/*** Operation table for the ISA bus device */

static struct oskit_isabus_ops ops = {
	query,
	addref,
	release,
	getinfo,
	getdriver,
	getchild,
	probe,
	getchildaddr,
	getchildio
};
static oskit_isabus_t bus = { &ops };

oskit_error_t
osenv_isabus_init(void)
{
	static oskit_bool_t initialized = FALSE;
	oskit_error_t err;

	if (initialized)
		return 0;

	err = osenv_rootbus_addchild((oskit_device_t*)&bus);
	if (err)
		return err;

	/*pnp_check();*/

	initialized = TRUE;
	return 0;
}

oskit_isabus_t *osenv_isabus_getbus(void)
{
	/* XXX */ osenv_isabus_init();

	return &bus;
}

oskit_error_t
osenv_isabus_addchild(oskit_u32_t addr, oskit_device_t *dev)
{
	oskit_error_t err;

	err = osenv_isabus_init();
	if (err)
		return err;

	return addchild(&bus, addr, dev);
}

oskit_error_t
osenv_isabus_remchild(oskit_u32_t addr)
{
	return remchild(&bus, addr);
}
