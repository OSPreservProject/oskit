/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * This routine repeatedly walks through the device tree
 * and calls all the buses' probe routines
 * until no more devices can be found.
 */

#include <string.h>

#include <oskit/dev/dev.h>
#include <oskit/dev/device.h>
#include <oskit/dev/bus.h>

static int probe(oskit_bus_t *bus)
{
	int found = 0;
	oskit_device_t *dev;
	oskit_bus_t *sub;
	char pos[OSKIT_BUS_POS_MAX+1];
	int i;

	/* Probe this bus */
	if (oskit_bus_probe(bus) > 0)
		found = 1;

	/* Find any child buses dangling from this bus */
	for (i = 0; oskit_bus_getchild(bus, i, &dev, pos) == 0; i++) {
		if (oskit_device_query(dev, &oskit_bus_iid, (void**)&sub) == 0)
			probe(sub);
		oskit_device_release(dev);
	}

	/* Release the reference to this bus node */
	oskit_bus_release(bus);

	return found;
}

void oskit_dev_probe(void)
{
	while (probe(osenv_rootbus_getbus()));
}

