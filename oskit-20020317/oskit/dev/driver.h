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
#ifndef _OSKIT_DEV_DRIVER_H_
#define _OSKIT_DEV_DRIVER_H_

#include <oskit/com.h>
#include <oskit/dev/error.h>

struct oskit_guid;

/*
 * Device information structure returned by oskit_driver->getinfo().
 * All string pointers returned are constant and valid
 * for the lifetime of the device node,
 * and should not be freed by the caller.
 * Any of the strings except 'name' may be NULL,
 * indicating that that information is not available.
 */
#define OSKIT_DEVNAME_MAX	15
struct oskit_devinfo {
	/*
	 * Short device type identifier, e.g., "aha1542".
	 * Must be no longer than OSKIT_DEVNAME_MAX characters long,
	 * and must contain only 7-bit alphanumeric characters;
	 * the first character must be alphabetic.
	 */
	const char	*name;

	/* Longer description of device, e.g., "Adaptec 1542 SCSI controller" */
	const char	*description;

	/* Vendor name of device, if available, e.g., "Adaptec" */
	const char	*vendor;

	/* Name of author of the device driver */
	const char	*author;

	/* Version/source of this device driver, e.g., "Linux 1.3.36" */
	const char	*version;	/* Origin and version of driver */
};
typedef struct oskit_devinfo oskit_devinfo_t;

/*
 * Definition of the basic oskit_driver interface,
 * IID 4aa7df88-7c74-11cf-b500-08000953adc2.
 * which represents a device driver class (as opposed to instance).
 * A given device driver can often control several actual devices.
 */
struct oskit_driver {
	struct oskit_driver_ops *ops;
};
typedef struct oskit_driver oskit_driver_t;

struct oskit_driver_ops {

	/* Inherited IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_driver_t *drv,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_driver_t *drv);
	OSKIT_COMDECL_U	(*release)(oskit_driver_t *drv);

	/*
	 * Return general information about this device driver,
	 * describing the types of devices it supports
	 * and the origin of the driver;
	 * see the description of oskit_devinfo_t above.
	 */
	OSKIT_COMDECL	(*getinfo)(oskit_driver_t *drv, oskit_devinfo_t *out_info);
};

extern const struct oskit_guid oskit_driver_iid;
#define OSKIT_DRIVER_IID OSKIT_GUID(0x4aa7df88, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_driver_query(drv, iid, out_ihandle) \
	((drv)->ops->query((oskit_driver_t *)(drv), (iid), (out_ihandle)))
#define oskit_driver_addref(drv) \
	((drv)->ops->addref((oskit_driver_t *)(drv)))
#define oskit_driver_release(drv) \
	((drv)->ops->release((oskit_driver_t *)(drv)))
#define oskit_driver_getinfo(drv, out_info) \
	((drv)->ops->getinfo((oskit_driver_t *)(drv), (out_info)))

/*** Entrypoints for device driver registration ***/
/*
 * These functions are the entrypoints to the driver registration module,
 * which does not itself need to be a COM object since there's only one.
 * The registration module is implemented in libfdev by default,
 * but can be overridden by the OS to provide a registration mechanism
 * specialized to the OS's needs and/or integrated with existing mechanisms.
 * For example, these functions could be overridden
 * to provide automatic on-demand loading of device driver modules.
 */

/*
 * Register a device driver as being available and ready to run (drive?).
 * The driver must provide a list of interface identifiers (IIDs)
 * for type-specific driver interfaces it supports (e.g., oskit_isa_driver)
 * in addition to oskit_driver_t which all drivers must support.
 */
oskit_error_t osenv_driver_register(oskit_driver_t *driver,
				  const struct oskit_guid *iids,
				  unsigned iid_count);

/*
 * Unregister a device driver registered with osenv_driver_register().
 * The parameters passed here must be _exactly_ the same
 * as the ones that were passed to osenv_driver_register() previously.
 */
oskit_error_t osenv_driver_unregister(oskit_driver_t *driver,
				    const struct oskit_guid *iids,
				    unsigned iid_count);

/*
 * Obtain a list of all the registered driver interfaces with a specified IID.
 * The oskit_driver_iid can be used to find _all_ registered drivers,
 * and other IIDs can be used to find all drivers of a specific type.
 * When the client is finished with the returned array,
 * it must release all the references it contains
 * and free the array itself using osenv_mem_free(arr, OSENV_AUTO_SIZE, 0).
 * Returns the number of interface pointers in the returned array;
 * if there are no matches in the database, returns 0
 * with *out_interface_array set to NULL.
 */
oskit_error_t osenv_driver_lookup(const struct oskit_guid *iid,
				void ***out_interface_array);

/*
 * Initialize the osenv_driver registration module. Should be called
 * from oskit_dev_init().
 */
void osenv_driver_registration_init(void);

#endif /* _OSKIT_DEV_DRIVER_H_ */
