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

#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/device.h>
#include <oskit/com/services.h>

/*
 * Local services database.
 */
static oskit_services_t		*device_registry;

oskit_error_t
osenv_device_register(oskit_device_t *device,
		     const struct oskit_guid *iids, unsigned iid_count)
{
	oskit_error_t rc;
	int i;

#ifndef KNIT
	osenv_assert(device_registry);
#endif

	/* Register the basic device interface */
	rc = oskit_services_addservice(device_registry,
				       &oskit_device_iid, device);
	if (rc)
		return rc;

	/* Register the other supported interfaces */
	for (i = 0; i < iid_count; i++) {
		oskit_iunknown_t *iu;
		rc = oskit_device_query(device, &iids[i], (void**)&iu);
		if (rc)
			return rc;
		rc = oskit_services_addservice(device_registry, &iids[i], iu);
		if (rc)
			return rc;
		oskit_iunknown_release(iu);
	}

	return 0;
}

oskit_error_t
osenv_device_unregister(oskit_device_t *device,
		       const struct oskit_guid *iids, unsigned iid_count)
{
	oskit_error_t rc;
	int i;

#ifndef KNIT
	osenv_assert(device_registry);
#endif

	/* Unregister the basic device interface */
	oskit_services_remservice(device_registry, &oskit_device_iid, device);

	/* Unregister the other supported interfaces */
	for (i = 0; i < iid_count; i++) {
		oskit_iunknown_t *iu;
		rc = oskit_device_query(device, &iids[i], (void**)&iu);
		if (rc)
			return rc;
		rc = oskit_services_remservice(device_registry, &iids[i], iu);
		if (rc)
			return rc;
		oskit_iunknown_release(iu);
	}

	return 0;
}

oskit_error_t
osenv_device_lookup(const struct oskit_guid *iid, void ***out_interface_array)
{
#ifndef KNIT
	osenv_assert(device_registry);
#endif

	return oskit_services_lookup(device_registry,
				     iid, out_interface_array);
}

/*
 * Initialize the device registration service. Create a services database
 * to hold all the goodies.
 */
#ifdef KNIT
oskit_error_t
init(void)
{
	return oskit_services_create((struct oskit_mem *) 0, &device_registry);
}
#else
void
osenv_device_registration_init(void)
{
	if (oskit_services_create((struct oskit_mem *) 0, &device_registry))
		osenv_panic("osenv_device_registration_init: Bad registry");
}
#endif
