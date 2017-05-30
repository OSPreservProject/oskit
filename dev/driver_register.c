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
#include <oskit/dev/driver.h>

/*
 * Local services database.
 */
static oskit_services_t		*driver_registry;

oskit_error_t
osenv_driver_register(oskit_driver_t *driver,
		     const struct oskit_guid *iids, unsigned iid_count)
{
	oskit_error_t rc;
	int i;

#ifdef KNIT
	osenv_assert(driver_registry);
#endif

	/* Register the basic driver interface */
	rc = oskit_services_addservice(driver_registry,
				       &oskit_driver_iid, driver);
	if (rc)
		return rc;

	/* Register the other supported interfaces */
	for (i = 0; i < iid_count; i++) {
		oskit_iunknown_t *iu;
		rc = driver->ops->query(driver, &iids[i], (void**)&iu);
		if (rc)
			return rc;
		rc = oskit_services_addservice(driver_registry, &iids[i], iu);
		if (rc)
			return rc;
		iu->ops->release(iu);
	}

	return 0;
}

oskit_error_t
osenv_driver_unregister(oskit_driver_t *driver,
		       const struct oskit_guid *iids, unsigned iid_count)
{
	oskit_error_t rc;
	int i;

#ifdef KNIT
	osenv_assert(driver_registry);
#endif

	/* Unregister the basic driver interface */
	oskit_services_remservice(driver_registry, &oskit_driver_iid, driver);

	/* Unregister the other supported interfaces */
	for (i = 0; i < iid_count; i++) {
		oskit_iunknown_t *iu;
		rc = driver->ops->query(driver, &iids[i], (void**)&iu);
		if (rc)
			return rc;
		rc = oskit_services_remservice(driver_registry, &iids[i], iu);
		if (rc)
			return rc;
		iu->ops->release(iu);
	}

	return 0;
}

oskit_error_t
osenv_driver_lookup(const struct oskit_guid *iid, void ***out_interface_array)
{
#ifdef KNIT
	osenv_assert(driver_registry);
#endif

	return oskit_services_lookup(driver_registry,
				     iid, out_interface_array);
}

/*
 * Initialize the driver registration service. Create a services database
 * to hold all the goodies.
 */
#ifdef KNIT
oskit_error_t
init(void)
{
	return oskit_services_create((struct oskit_mem *) 0, &driver_registry);
}
#else
void
osenv_driver_registration_init(void)
{
	if (oskit_services_create((struct oskit_mem *) 0, &driver_registry))
		osenv_panic("osenv_driver_registration_init: Bad registry");
}
#endif
