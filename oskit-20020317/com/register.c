/*
 * Copyright (c) 1996-1999 University of Utah and the Flux Group.
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
 * This is the global registry. It is just a services database.
 */

#include <oskit/com.h>
#include <oskit/com/services.h>
#include <oskit/com/mem.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/assert.h>

static oskit_services_t		*global_registry;

/*
 * Create the global registry and remember it. The caller must provide
 * the memory object to be used by the services code. 
 */
oskit_error_t
oskit_global_registry_create(oskit_mem_t *memobject)
{
	oskit_error_t	rc;

	assert(memobject);
	
	if ((rc = oskit_services_create(memobject, &global_registry)))
		return rc;

	return 0;
}

/*
 * The rest of these routines map to the services interface.
 */
oskit_error_t
oskit_register(const struct oskit_guid *iid, void *interface)
{
	assert(global_registry);
	
	return oskit_services_addservice(global_registry, iid, interface);
}

oskit_error_t
oskit_unregister(const struct oskit_guid *iid, void *interface)
{
	assert(global_registry);
	
	return oskit_services_remservice(global_registry, iid, interface);
}

oskit_error_t
oskit_lookup(const oskit_guid_t *iid, void ***out_interface_array)
{
	assert(global_registry);
	
	return oskit_services_lookup(global_registry,
				     iid, out_interface_array);
}

oskit_error_t
oskit_lookup_first(const oskit_guid_t *iid, void **out_intf)
{
	assert(global_registry);
	
	return oskit_services_lookup_first(global_registry, iid, out_intf);
}

/*
 * Return a reference the global registry. This function should be deprecated.
 */
oskit_services_t *
oskit_get_services(void)
{
	return global_registry;
}
