/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * Default implementation of the root device node "pseudo-bus".
 */

#include <stdio.h>
#include <string.h>
#include <oskit/debug.h>
#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/device.h>
#include <oskit/dev/bus.h>

/* List of device nodes attached to the root bus */
static struct node {
	struct node *next;
	oskit_device_t *dev;
	char pos[OSKIT_BUS_POS_MAX];
} *nodes;

static OSKIT_COMDECL
query(oskit_bus_t *bus, const oskit_iid_t *iid, void **out_ihandle)
{
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_driver_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_device_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_bus_iid, sizeof(*iid)) == 0) {
		*out_ihandle = bus;
		return 0;
	}
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
addref(oskit_bus_t *bus)
{
	return 1;
}

static OSKIT_COMDECL_U
release(oskit_bus_t *bus)
{
	return 1;
}

static OSKIT_COMDECL
getinfo(oskit_bus_t *bus, oskit_devinfo_t *out_info)
{
	out_info->name = "root";
	out_info->description = "Hardware Root";
	out_info->vendor = NULL;
	out_info->author = "Flux Project, University of Utah";
	out_info->version = NULL; /* OSKIT_VERSION or somesuch? */
	return 0;
}

static OSKIT_COMDECL
getdriver(oskit_bus_t *dev, oskit_driver_t **out_driver)
{
	/*
	 * Our driver node happens to be the same COM object
	 * as our device node, making this really easy...
	 */
	*out_driver = (oskit_driver_t*)dev;
	return 0;
}

static OSKIT_COMDECL
getchild(oskit_bus_t *bus, unsigned idx,
	  struct oskit_device **out_fdev, char *out_pos)
{
	struct node *n;

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
	strcpy(out_pos, n->pos);
	return 0;
}

static OSKIT_COMDECL
probe(oskit_bus_t *bus)
{
	return OSKIT_E_NOTIMPL;
}

static struct oskit_bus_ops ops = {
	query,
	addref,
	release,
	getinfo,
	getdriver,
	getchild,
	probe
};
static oskit_bus_t bus = { &ops };

struct oskit_bus *
osenv_rootbus_getbus(void)
{
	return &bus;
}

oskit_error_t
osenv_rootbus_addchild(oskit_device_t *dev)
{
	struct node *n, *nn;
	oskit_devinfo_t info;
	oskit_error_t err;
	int pos_n;

	/* XXX check already present */

	/*
	 * Find the device's short name;
	 * we'll use that to generate the position string.
	 */
	err = oskit_device_getinfo(dev, &info);
	if (err)
		return err;

	/* Create a new device node */
	n = osenv_mem_alloc(sizeof(*n), OSENV_PHYS_WIRED, 0);
	if (n == NULL)
		return OSKIT_E_OUTOFMEMORY;
	n->next = nodes; nodes = n;
	n->dev = dev; oskit_device_addref(dev);

	/*
	 * Produce a unique position string for this device node.
	 * If there are multiple nodes with the same name,
	 * tack on an integer to make them unique.
	 */
	osenv_assert(OSKIT_DEVNAME_MAX < OSKIT_BUS_POS_MAX);
	osenv_assert(strlen(info.name) <= OSKIT_DEVNAME_MAX);
	pos_n = 0;
	strcpy(n->pos, info.name);
	retry:
	for (nn = n->next; nn; nn = nn->next) {
		if (strcmp(n->pos, nn->pos) == 0) {
			sprintf(n->pos, "%s%d", info.name, ++pos_n);
			goto retry;
		}
	}

	return 0;
}

oskit_error_t
osenv_rootbus_remchild(oskit_device_t *dev)
{
	struct node **np, *n;

	for (np = &nodes; (n = *np) != NULL; np = &n->next) {
		/*
		 * XXX Technically this isn't correct COM procedure;
		 * we should be comparing the IUnknown interfaces...
		 */
		if (n->dev == dev) {
			*np = n->next;
			oskit_device_release(n->dev);
			osenv_mem_free(n, OSENV_PHYS_WIRED, sizeof(*n));
			return 0;
		}
	}

	return OSKIT_E_DEV_NOSUCH_CHILD;
}

