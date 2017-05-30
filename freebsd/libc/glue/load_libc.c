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

#include <stdlib.h>
#include <oskit/c/fd.h>
#include <oskit/c/environment.h>

oskit_services_t	*libc_services_object;
oskit_libcenv_t		*libc_environment;

/*
 * Create the C library. This is sorta like initializing, but not really.
 * This will also be used when dynamically loading a program; a pointer
 * to an oskit services database will be provided, from which the C library
 * can ask for all the external interfaces it needs. 
 */
void
oskit_load_libc(oskit_services_t *services)
{
	libc_services_object = services;

	oskit_services_lookup_first(libc_services_object,
				    &oskit_libcenv_iid,
				    (void *) &libc_environment);

	if (!libc_environment)
		panic("oskit_load_libc: "
		      "Could not find the libc environment object!");
}
