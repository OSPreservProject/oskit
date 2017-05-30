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

#include <oskit/dev/osenv.h>
#include "osenv.h"

oskit_osenv_sleep_t		*fs_netbsd_oskit_osenv_sleep;
oskit_osenv_intr_t		*fs_netbsd_oskit_osenv_intr;
oskit_osenv_log_t		*fs_netbsd_oskit_osenv_log;
oskit_osenv_mem_t		*fs_netbsd_oskit_osenv_mem;
oskit_osenv_timer_t		*fs_netbsd_oskit_osenv_timer;

#define GETIFACE(services, name) ({					    \
	oskit_services_lookup_first((services),				    \
				&(oskit_osenv_##name##_iid),		    \
				(void **) &(fs_netbsd_oskit_osenv_##name)); \
	if (! fs_netbsd_oskit_osenv_##name)				    \
		osenv_panic("fs_netbsd_oskit_osenv_init: "		    \
			    "No registered interface for osenv_" #name);    \
})

/*
 * Initialize the osenv glue. These are all the interfaces that the
 * netbsd code needs. Right now this is shared. It might be that it
 * should be split into fs and dev specific portions, since the fs code
 * will usually require less stuff. 
 */
void
fs_netbsd_oskit_osenv_init(oskit_services_t *osenv)
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
	GETIFACE(osenv, mem);
	GETIFACE(osenv, timer);
}
