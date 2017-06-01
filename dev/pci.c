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
 * This provides the osenv interfaces for access to PCI
 * configuration space.
 */

#include <oskit/dev/dev.h>

/*
 * These are the machine-dependant routines.
 * May either program configuration cycles directly or call into
 * BIOS32 (on x86 anyway).
 */
int pci_config_read(char bus, char device, char function, char port,
	unsigned *data);

int pci_config_write(char bus, char device, char function, char port,
	unsigned data);

int pci_config_init();


int
osenv_pci_config_read(char bus, char device, char function, char port,
	unsigned *data)
{
	osenv_assert((port & 0x3) == 0);

	return pci_config_read(bus, device, function, port, data);
}


int
osenv_pci_config_write(char bus, char device, char function, char port,
	unsigned data)
{
	osenv_assert((port & 0x3) == 0);
	return pci_config_write(bus, device, function, port, data);
}


/*
 * This returns zero on success, non-zero on failure (ie, no PCI).
 */
int
osenv_pci_config_init()
{
	return pci_config_init();
}

