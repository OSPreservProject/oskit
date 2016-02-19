/*
 * Copyright (c) 1998-2000 The University of Utah and the Flux Group.
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

#include <oskit/dev/dev.h>
#include <oskit/c/environment.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_intr.h>

/*
 * The oskit_interrupt object, which provides the hooks to interrupt
 * enable/disable in both single and multi-threaded mode. 
 */
oskit_osenv_intr_t	*posixlib_osenv_intr_iface;

void
posixlib_osenv_intr_init_iface(void)
{
	int			enabled;
	oskit_osenv_t		*osenv;
	oskit_osenv_intr_t	*iface;

	if (!posixlib_osenv_intr_iface) {
		/*
		 * Find the osenv first.
		 */
		oskit_services_lookup_first(libc_services_object,
					    &oskit_osenv_iid, (void *) &osenv);

		/*
		 * Must not be ready yet!
		 */
		if (!osenv)
			return;

		oskit_osenv_lookup(osenv,
				   &oskit_osenv_intr_iid, (void *) &iface);

		if (iface) {
			enabled = oskit_osenv_intr_save_disable(iface);

			posixlib_osenv_intr_iface = iface;

			if (enabled)
				oskit_osenv_intr_enable(iface);
		}

		oskit_osenv_release(osenv);
	}
}

