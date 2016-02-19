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
 * Locking?
 */

/*
 * COM interface for the default clientos.
 */
#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/osenv_intr.h>
#include <oskit/com/services.h>
#include <oskit/com/libcenv.h>
#include <oskit/clientos.h>
#include <oskit/dev/osenv_sleep.h>
#include <oskit/com/mem.h>
#include <fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef PTHREADS
#include <oskit/threads/pthread.h>
#define oskit_clientos_init	oskit_clientos_init_pthreads
#define	oskit_libcenv_create	oskit_libcenv_create_pthreads
#endif

/*
 * The default client OS requires the memory object. With that, we can
 * create the global registry. Since the rest of the system requires the
 * memory object, install that in the global registry for components
 * to find.
 *
 * With that done, we can install the libc environment object. It is
 * not fully initialized yet, but thats okay since the kernel first
 * needs to get all the right stuff in place, and there are cross
 * dependencies. For example, to get the network started, the filesystem
 * needs to be in place.
 *
 * Then hand the services database off to the C library so it can do some
 * internal initialization using the registry.
 */
oskit_error_t
oskit_clientos_init(void)
{
	oskit_libcenv_t		*libcenv;
	oskit_mem_t		*memi;

	/*
	 * Get the initial memory object.
	 */
	memi = oskit_mem_init();
	assert(memi);

	/*
	 * Create the global registry, parameterized with the memory object.
	 */
	if (oskit_global_registry_create(memi))
		panic("oskit_clientos_init: Problem creating global registry");

	/*
	 * Install the memory object in the global registry for components
	 * to find as needed.
	 */
	oskit_register(&oskit_mem_iid, (void *) memi);

	/*
	 * Create the libc environment and install it in the global registry.
	 */
	oskit_libcenv_create(&libcenv);
	oskit_register(&oskit_libcenv_iid, (void *) libcenv);

	/*
	 * Now "load" the C library, giving it a pointer to the services
	 * database. From there, it can get everything else it needs.
	 */
	oskit_load_libc(oskit_get_services());

	return 0;
}
