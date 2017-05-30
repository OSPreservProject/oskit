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
 * Driver initialization.
 */

#include <oskit/dev/dev.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/driver.h>
#include <oskit/dev/device.h>
#include <oskit/debug.h>

void
oskit_dev_init(oskit_osenv_t *osenv)
{
	static int fdev_inited = 0;

	/* Only initialize once! */
	if (fdev_inited)
		return;
	else
		fdev_inited = 1;

	osenv_device_registration_init();
	osenv_driver_registration_init();
}
