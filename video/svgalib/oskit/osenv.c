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
#include <oskit/com/services.h>
#include "osenv.h"

oskit_osenv_intr_t		*svgalib_oskit_osenv_intr;
oskit_osenv_mem_t		*svgalib_oskit_osenv_mem;
oskit_osenv_log_t		*svgalib_oskit_osenv_log;

#define GETIFACE(services, name) ({					    \
	oskit_services_lookup_first((services),				    \
				&(oskit_osenv_##name##_iid),	            \
				(void **) &(svgalib_oskit_osenv_##name));   \
	if (! svgalib_oskit_osenv_##name)				    \
		osenv_panic("oskit_svgalib_osenv_init: "		    \
			    "No registered interface for osenv_" #name);    \
})

/*
 * Initialize the osenv glue. These are all the interfaces that the
 * svgalib code needs. Right now this is shared. It might be that it
 * should be split into fs and dev specific portions, since the fs code
 * will usually require less stuff. 
 */
void
oskit_svgalib_osenv_init(void)
{
	static int inited = 0;
	oskit_osenv_t	*osenv;

	if (inited)
		return;
	inited = 1;

	/*
	 * Find the osenv, and then the various osenv interfaces this
	 * component needs.
	 */
	oskit_lookup_first(&oskit_osenv_iid, (void *) &osenv);

	/*
	 * Now get each one. Its fatal if one we need is not available.
	 */
	GETIFACE(osenv, log);
	GETIFACE(osenv, intr);
	GETIFACE(osenv, mem);
}
