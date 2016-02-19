/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * Convenience function to initialize/probe device drivers,
 * using link-time dependencies to select sets to initialize.
 */

#include <oskit/startup.h>
#include <oskit/dev/dev.h>
#include <oskit/c/stdio.h>
#include <oskit/clientos.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_sleep.h>

#ifdef DEBUG
static int verbose = 1;
#else
static int verbose = 0;
#endif

/*
 * These are "common" (uninitialized) definitions so that other files
 * that get linked in containing initialized definitions will override
 * them.  That way, if the program calls start_disk (because it uses the disk),
 * then start_disk.c's definition of _init_devices_blk will make start_devices
 * start the block (disk) device drivers.  Likewise, start_network.c defines
 * _init_devices_net so the network drivers will be initialized if the
 * program uses the network.
 */
void (*_init_devices_blk)();
void (*_init_devices_net)();

void
start_devices(void)
{
	oskit_osenv_t *osenv;
	static int run;
	
	if (run)
		return;
	run = 1;

	if (verbose)
		printf("Initializing device drivers...\n");
	osenv = start_osenv();
	oskit_dev_init(osenv);
	oskit_linux_init_osenv(osenv);

	if (_init_devices_blk)
		(*_init_devices_blk)();
	if (_init_devices_net)
		(*_init_devices_net)();

	if (verbose)
		oskit_dump_drivers();

	if (verbose)
		printf("Probing devices...\n");
	oskit_dev_probe();

	if (verbose)
		oskit_dump_devices();
}
