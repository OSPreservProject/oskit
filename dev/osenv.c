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
 * Default OS environment. Gather up all the little interfaces, and
 * put them into a osenv (services) structure, which is then used to
 * parameterize the driver code.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_irq.h>
#include <oskit/dev/osenv_pci_config.h>
#include <oskit/dev/osenv_isa.h>
#include <oskit/dev/osenv_log.h>
#include <oskit/dev/osenv_mem.h>
#include <oskit/dev/osenv_driver.h>
#include <oskit/dev/osenv_device.h>
#include <oskit/dev/osenv_ioport.h>
#include <oskit/dev/osenv_timer.h>
#include <oskit/dev/osenv_sleep.h>
#include <oskit/dev/osenv_rtc.h>

oskit_osenv_t *
oskit_osenv_create_default(void)
{
	oskit_osenv_t *osenv;

	osenv = oskit_osenv_create();
	if (! osenv)
		osenv_panic("Could not allocate the OS environment object");

	INIT_OSENV_IFACE(osenv, log);
	INIT_OSENV_IFACE(osenv, intr);
	INIT_OSENV_IFACE_PARMED(osenv, sleep, oskit_create_osenv_intr());
	INIT_OSENV_IFACE(osenv, irq);
	INIT_OSENV_IFACE(osenv, pci_config);
	INIT_OSENV_IFACE(osenv, isa);
	INIT_OSENV_IFACE(osenv, mem);
	INIT_OSENV_IFACE(osenv, driver);
	INIT_OSENV_IFACE(osenv, device);
	INIT_OSENV_IFACE(osenv, ioport);
	INIT_OSENV_IFACE(osenv, timer);
	INIT_OSENV_IFACE(osenv, rtc);

	return osenv;
}
