/*
 * Copyright (c) 2001 University of Utah and the Flux Group.
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
 * Convenience function to initialize sound device drivers.
 * XXX should be integrated with start_devices.
 */

#include <oskit/startup.h>
#include <oskit/dev/dev.h>
#include <oskit/clientos.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/osenv.h>

void
start_sound_devices(void)
{
	oskit_osenv_t *osenv;
	static int run;
	
	if (run)
		return;
	run = 1;

	osenv = start_osenv();
	oskit_dev_init(osenv);
	oskit_linux_init_osenv(osenv);
	oskit_linux_init_sound();
}
