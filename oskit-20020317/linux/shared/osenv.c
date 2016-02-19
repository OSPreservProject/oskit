/*
 * Copyright (c) 1999 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

#include <oskit/dev/osenv.h>
#include "shared.h"
#include "osenv.h"

oskit_osenv_sleep_t		*linux_oskit_osenv_sleep;
oskit_osenv_intr_t		*linux_oskit_osenv_intr;
oskit_osenv_irq_t		*linux_oskit_osenv_irq;
oskit_osenv_pci_config_t	*linux_oskit_osenv_pci_config;
oskit_osenv_isa_t		*linux_oskit_osenv_isa;
oskit_osenv_log_t		*linux_oskit_osenv_log;
oskit_osenv_mem_t		*linux_oskit_osenv_mem;
oskit_osenv_driver_t		*linux_oskit_osenv_driver;
oskit_osenv_device_t		*linux_oskit_osenv_device;
oskit_osenv_ioport_t		*linux_oskit_osenv_ioport;
oskit_osenv_timer_t		*linux_oskit_osenv_timer;

#define GETIFACE(services, name) ({					    \
	oskit_services_lookup_first((services),				    \
				    &(oskit_osenv_##name##_iid),	    \
				    (void **) &(linux_oskit_osenv_##name)); \
	if (! linux_oskit_osenv_##name)					    \
		osenv_log(0, "oskit_linux_osenv_init: "			    \
			  "No registered interface for osenv_" #name "\n"); \
})

/*
 * Initialize the osenv glue. These are all the interfaces that the
 * linux code needs. Right now this is shared. It might be that it
 * should be split into fs and dev specific portions, since the fs code
 * will usually require less stuff. 
 */
void
oskit_linux_osenv_init(oskit_services_t	*osenv)
{
	static int inited = 0;

	if (inited)
		return;
	inited = 1;

	/*
	 * Now get each one. Its fatal if one we need is not available.
	 * Better get the log/panic interface first though!
	 */
	GETIFACE(osenv, log);
	GETIFACE(osenv, sleep);
	GETIFACE(osenv, intr);
	GETIFACE(osenv, irq);
	GETIFACE(osenv, pci_config);
	GETIFACE(osenv, isa);
	GETIFACE(osenv, mem);
	GETIFACE(osenv, driver);
	GETIFACE(osenv, device);
	GETIFACE(osenv, ioport);
	GETIFACE(osenv, timer);
}
