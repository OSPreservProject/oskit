/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
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
 * Default memory bus "device driver",
 * representing the local processor bus on which we are running.
 */

#include <stdio.h>
#include <oskit/debug.h>
#include <oskit/boolean.h>
#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/device.h>
#include <oskit/dev/bus.h>
#include <oskit/dev/membus.h>
#include <oskit/c/string.h>

/* List of child devices attached to this bus, in no particular order */
static struct busnode {
	struct busnode *next;
	oskit_addr_t addr;
	oskit_device_t *dev;
} *nodes;

#if 0
/* List of address ranges allocated, in no particular order */
static struct arange {
	struct arange *next;
	oskit_addr_t start, end;
	oskit_device_t *dev;
} *ports;
#endif /* 0 */

static OSKIT_COMDECL
query(oskit_membus_t *bus, const oskit_iid_t *iid, void **out_ihandle)
{
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_driver_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_device_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_bus_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_membus_iid, sizeof(*iid)) == 0) {
		*out_ihandle = bus;
		return 0;
	}
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
addref(oskit_membus_t *bus)
{
	return 1;
}

static OSKIT_COMDECL_U
release(oskit_membus_t *bus)
{
	return 1;
}

static OSKIT_COMDECL
getinfo(oskit_membus_t *bus, oskit_devinfo_t *out_info)
{
	out_info->name = "mem";
	out_info->description = "Generic Memory Bus";
	out_info->vendor = NULL;
	out_info->author = "Flux Project, University of Utah";
	out_info->version = NULL; /* OSKIT_VERSION or somesuch? */
	return 0;
}

static OSKIT_COMDECL
getdriver(oskit_membus_t *dev, oskit_driver_t **out_driver)
{
	/*
	 * Our driver node happens to be the same COM object
	 * as our device node, making this really easy...
	 */
	*out_driver = (oskit_driver_t*)dev;
	return 0;
}

static OSKIT_COMDECL
probe(oskit_membus_t *bus)
{
	/* No probe support at this point */
	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
getchildaddr(oskit_membus_t *bus, oskit_u32_t idx, oskit_device_t **out_fdev,
		oskit_addr_t *out_addr)
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
getchild(oskit_membus_t *bus, oskit_u32_t idx, oskit_device_t **out_fdev,
	  char *out_pos)
{
	oskit_error_t err;
	oskit_addr_t addr;

	err = getchildaddr(bus, idx, out_fdev, &addr);
	if (err)
		return err;

	if (sizeof(addr) == 4)
		sprintf(out_pos, "0x%08x", addr);
	else
		sprintf(out_pos, "0x%16x", addr);

	return 0;
}

#if 0	/* XXX mem_bus_io interface not fully implemented yet */

/*** Bus device registration ***/

static oskit_error_t
add_child(oskit_membus_t *bus, oskit_addr_t addr, oskit_device_t *dev)
{
	struct busnode *n;

	/* XXX sanity check params */
	/* XXX check for addr collisions */

	/* Create a new device node */
	n = osenv_mem_alloc(sizeof(*n), OSENV_PHYS_WIRED, 0);
	if (n == NULL)
		return OSKIT_E_OUTOFMEMORY;
	n->next = nodes; nodes = n;
	n->dev = dev; oskit_device_addref(dev);
	n->addr = addr;

	return 0;
}

static oskit_error_t
remove_child(oskit_membus_t *bus, oskit_addr_t addr)
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


/*** Address space allocation ***/

static oskit_bool_t addr_avail(oskit_membus_t *bus,
			      oskit_addr_t start, oskit_size_t size)
{
	struct arange *r;
	oskit_addr_t end = start + size;

	for (r = ports; r; r = r->next) {
		if (start < r->end && end > r->start)
			return 0;
	}
	return 1;
}

static oskit_error_t addr_alloc(oskit_membus_t *bus,
			       oskit_addr_t start, oskit_size_t size,
			       oskit_device_t *owner)
{
	struct arange *r;
	oskit_addr_t end = start + size;

	if (!addr_avail(bus, start, size))
		return OSKIT_E_DEV_SPACE_INUSE;

	r = osenv_mem_alloc(sizeof(*r), OSENV_PHYS_WIRED, 0);
	if (r == NULL)
		return OSKIT_E_OUTOFMEMORY;
	r->next = ports; ports = r;
	r->dev = owner; oskit_device_addref(owner);
	r->start = start;
	r->end = end;

	return 0;
}

static oskit_error_t addr_free(oskit_membus_t *bus,
			      oskit_addr_t start, oskit_size_t size)
{
	struct arange **rp, *r;
	oskit_addr_t end = start + size;

	for (rp = &ports; (r = *rp) != NULL; rp = &r->next) {
		if (r->start == start && r->end == end) {
			*rp = r->next;
			oskit_device_release(r->dev);
			osenv_mem_free(r, OSENV_PHYS_WIRED, sizeof(*r));
			return 0;
		}
	}

	return OSKIT_E_DEV_SPACE_UNASSIGNED;
}

static oskit_error_t addr_scan(oskit_membus_t *bus,
			      oskit_addr_t *inout_addr, oskit_size_t *out_size,
			      oskit_device_t **out_owner)
{
	struct arange *r, *f = NULL;
	oskit_addr_t addr = *inout_addr;

	/* Find the first range starting from the specified start address */
	for (r = ports; r; r = r->next) {
		if (r->end > addr &&
		    (f == NULL || r->start < f->start))
			f = r;
	}

	/* If no such port was found, return an indication */
	if (f == NULL) {
		*out_owner = NULL;
		return OSKIT_S_FALSE;
	}

	*inout_addr = f->start;
	*out_size = f->end - f->start;
	*out_owner = f->dev; oskit_device_addref(f->dev);
	return OSKIT_S_TRUE;
}

/*** Physical Memory Access ***/

static oskit_u8_t
read8(oskit_membus_t *bus, oskit_addr_t addr)
{
	otsan();
}

static oskit_u16_t
read16(oskit_membus_t *bus, oskit_addr_t addr)
{
	otsan();
}

static oskit_u32_t
read32(oskit_membus_t *bus, oskit_addr_t addr)
{
	otsan();
}

static oskit_u64_t
read64(oskit_membus_t *bus, oskit_addr_t addr)
{
	otsan();
}

static void
write8(oskit_membus_t *bus, oskit_addr_t addr, oskit_u8_t val)
{
	otsan();
}

static void
write16(oskit_membus_t *bus, oskit_addr_t addr, oskit_u16_t val)
{
	otsan();
}

static void
write32(oskit_membus_t *bus, oskit_addr_t addr, oskit_u32_t val)
{
	otsan();
}

static void
write64(oskit_membus_t *bus, oskit_addr_t addr, oskit_u64_t val)
{
	otsan();
}

/*** Physical Memory Mapping ***/

static oskit_error_t
map(oskit_membus_t *bus, oskit_addr_t physaddr, oskit_size_t size,
    oskit_u32_t flags, void **out_virtaddr)
{
	otsan();
}

static oskit_error_t
unmap(oskit_membus_t *bus, void *virtaddr, oskit_size_t size)
{
	otsan();
}

#endif /* 0 */

/*** Operation table for the memory bus device */

static struct oskit_membus_ops ops = {
	query,
	addref,
	release,
	getinfo,
	getdriver,
	getchild,
	probe,
	getchildaddr
};
static oskit_membus_t bus = { &ops };

oskit_error_t
osenv_membus_init(void)
{
	static oskit_bool_t initialized = FALSE;
	oskit_error_t err;

	if (initialized)
		return 0;

	err = osenv_rootbus_addchild((oskit_device_t*)&bus);
	if (err)
		return err;

	initialized = TRUE;
	return 0;
}

oskit_membus_t *osenv_membus_getbus(void)
{
	return &bus;
}

