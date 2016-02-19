/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Dump a summary of the fdev hardware tree.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/device.h>
#include <oskit/dev/bus.h>

void oskit_dump_drivers(void)
{
	oskit_error_t rc;
	oskit_driver_t **drivers;
	int i, count;

	osenv_log(OSENV_LOG_INFO, "Currently registered device drivers:\n");

	rc = osenv_driver_lookup(&oskit_driver_iid, (void***)&drivers);
	if (OSKIT_FAILED(rc) || rc == 0)
		return;
	count = rc;

	for (i = 0; i < count; i++) {
		oskit_devinfo_t info;
		rc = oskit_driver_getinfo(drivers[i], &info);
		if (OSKIT_SUCCEEDED(rc)) {
			osenv_log(OSENV_LOG_INFO, "  %-16s%s\n", info.name,
				info.description ? info.description : 0);
		}
		oskit_driver_release(drivers[i]);
	}

	/* Note that osenv_driver_lookup uses malloc, not osenv_mem_alloc */
	free(drivers);
}

#define LINE_LEN	80
#define INDENT		2
#define INDENT_MAX	20
#define DESCR_POS	30

static void dumpbus(oskit_bus_t *bus, int indent)
{
	oskit_device_t *dev;
	oskit_devinfo_t info;
	oskit_bus_t *sub;
	char pos[OSKIT_BUS_POS_MAX+1];
	char buf[LINE_LEN];
	int i, j;

	if (indent < INDENT_MAX)
		indent += INDENT;

	for (i = 0; bus->ops->getchild(bus, i, &dev, pos) == 0; i++) {

		/* Create an info line for this device */
		memset(buf, ' ', LINE_LEN-1);
		buf[LINE_LEN-1] = 0;
		memcpy(buf+indent, pos, j = strlen(pos));

		/* Add some more descriptive info to the info line */
		if (dev->ops->getinfo(dev, &info) == 0) {
			buf[indent+j] = '.';
			memcpy(buf+indent+j+1, info.name,
				strlen(info.name));
			strncpy(buf+DESCR_POS, info.description,
				LINE_LEN-DESCR_POS-1);
		}

		/* Output the line */
		osenv_log(OSENV_LOG_INFO, "%s\n", buf);

		/* If this is a bus, descend into it */
		if (dev->ops->query(dev, &oskit_bus_iid, (void**)&sub) == 0)
			dumpbus(sub, indent);

		/* Release the reference to this device node */
		dev->ops->release(dev);
	}

	/* Release the reference to this bus node */
	bus->ops->release(bus);
}

void oskit_dump_devices(void)
{
	osenv_log(OSENV_LOG_INFO, "Current hardware tree:\n");
	dumpbus(osenv_rootbus_getbus(), 0);
}
