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

#include <stdlib.h>
#include <oskit/dev/osenv.h>
#include "osenv.h"

oskit_osenv_intr_t		*oskit_x11_oskit_osenv_intr;
oskit_osenv_mem_t		*oskit_x11_oskit_osenv_mem;

#define GETIFACE(services, name) ({					    \
	oskit_services_lookup_first((services),				    \
				&(oskit_osenv_##name##_iid),	            \
				(void **) &(oskit_x11_oskit_osenv_##name)); \
	if (! oskit_x11_oskit_osenv_##name)				    \
		panic("oskit_x11_osenv_init: "			            \
		      "No registered interface for osenv_" #name "\n");     \
})

/*
 * Initialize the osenv glue. These are all the interfaces that the
 * x11 code needs.
 */
void
oskit_x11_osenv_init(void)
{
	static int inited = 0;
	oskit_osenv_t *osenv = 0;

	if (inited)
		return;
	inited = 1;

	/*
	 * Find the osenv
	 */
	oskit_lookup_first(&oskit_osenv_iid, (void *) &osenv);

	/*
	 * Now get each one. Its fatal if one we need is not available.
	 */
	GETIFACE(osenv, intr);
	GETIFACE(osenv, mem);
}
