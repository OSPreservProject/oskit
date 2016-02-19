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
 * Convenience function to create the default OS environment. 
 */

#include <oskit/startup.h>
#include <oskit/dev/dev.h>
#include <oskit/c/stdio.h>
#include <oskit/clientos.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_sleep.h>

/*
 * This is the osenv object created by the startup library. Since the
 * startup library is a convenience library, it is okay to use a single big
 * object for all of the various device driver packages. The kernel writer
 * is free to customize this. 
 */
static oskit_osenv_t *osenv;

/*
 * Subsequent callers of start_osenv just want a reference.
 */
oskit_osenv_t *
start_osenv(void)
{
	if (osenv)
		return osenv;

#ifdef DEBUG
	printf("Creating OS environment\n");
#endif
	osenv = oskit_osenv_create_default();

	/*
	 * Install the osenv in the global registry. This is the method by
	 * which most components will find the osenv database. At some
	 * point we may change everything to take the osenv (or particular
	 * osenv interfaces) as a parameter, but until then, this is an
	 * easy way to transition.
	 */
	oskit_register(&oskit_osenv_iid, (void *) osenv);

	return osenv;
}
