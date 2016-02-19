/*
 * Copyright (c) 1997-2001 University of Utah and the Flux Group.
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

#include <oskit/dev/linux.h>
#include <oskit/startup.h>

/*
 * Override OSKit defaults.
 *
 * Tweak around in here to include only specific block or network device
 * drivers.  Choices for block devices include IDE and various SCSI
 * controllers listed in oskit/dev/linux_scsi.h.  Choices for network
 * devices include all those listed in oskit/dev/linux_ethernet.h.
 */
static void
my_net_devices(void)
{
#ifdef UTAHTESTBED
	oskit_linux_init_ethernet_tulip();
	oskit_linux_init_ethernet_eepro100();
#else
	oskit_linux_init_net();
#endif
}

static void
my_blk_devices(void)
{
#ifdef UTAHTESTBED
	oskit_linux_init_ide();
#else
	oskit_linux_init_blk();
#endif
}

extern void (*_init_devices_net)(void);
extern void (*_init_devices_blk)(void);

void
start_net_devices(void)
{
	_init_devices_net = my_net_devices;
	_init_devices_blk = my_blk_devices;
	start_devices();
}

void
start_blk_devices(void)
{
	_init_devices_net = my_net_devices;
	_init_devices_blk = my_blk_devices;
	start_devices();
}
