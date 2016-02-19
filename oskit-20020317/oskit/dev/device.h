
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
#ifndef _OSKIT_DEV_DEVICE_H_
#define _OSKIT_DEV_DEVICE_H_

#include <oskit/com.h>
#include <oskit/dev/driver.h>

/*
 * Description of a COM device (instance) or device driver (class) object.
 * COM interface ID: 4aa7df80-7c74-11cf-b500-08000953adc2
 * This is only the part visible to clients, containing the vtable;
 * the actual opaque object state is stored after this structure.
 *
 * An object exporting this interface represents a device instance.
 * A given device driver can often control several actual devices;
 * such a driver will have one driver node but several device nodes.
 * Although oskit_device technically isn't a subtype of oskit_driver,
 * device drivers can implement them as such if it is convenient to do so,
 * effectively making the device node and driver node the same object.
 */
struct oskit_device {
	struct	oskit_device_ops *ops;	/* driver operations vector */
};
typedef struct oskit_device oskit_device_t;

/*
 * Device operations vector table.
 */
struct oskit_device_ops {

	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_device_t *dev,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_device_t *dev);
	OSKIT_COMDECL_U	(*release)(oskit_device_t *dev);

	/*
	 * Return specific information on this device and its driver.
	 * Note that although this call uses the same oskit_devinfo_t structure
	 * as the corresponding oskit_driver->getinfo() call on driver nodes,
	 * the device node and its corresponding driver node
	 * may return somewhat different information:
	 * for example, the driver node may say "aha15xx",
	 * indicating that the driver supports a whole series of devices,
	 * whereas the device node may report the specific device attached,
	 * such as "aha1542".
	 */
	OSKIT_COMDECL	(*getinfo)(oskit_device_t *dev, oskit_devinfo_t *out_info);

	/*
	 * Return a reference to the driver (class) node
	 * for this device (instance).
	 */
	OSKIT_COMDECL	(*getdriver)(oskit_device_t *dev,
				      oskit_driver_t **out_driver);
};

extern const struct oskit_guid oskit_device_iid; /* GUID for device interface */
#define OSKIT_DEVICE_IID OSKIT_GUID(0x4aa7df80, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_device_query(dev, iid, out_ihandle) \
	((dev)->ops->query((oskit_device_t *)(dev), (iid), (out_ihandle)))
#define oskit_device_addref(dev) \
	((dev)->ops->addref((oskit_device_t *)(dev)))
#define oskit_device_release(dev) \
	((dev)->ops->release((oskit_device_t *)(dev)))
#define oskit_device_getinfo(dev, out_info) \
	((dev)->ops->getinfo((oskit_device_t *)(dev), (out_info)))
#define oskit_device_getdriver(dev, out_driver) \
	((dev)->ops->getdriver((oskit_device_t *)(dev), (out_driver)))

/*** Entrypoints for device interface registration ***/
/*
 * These functions are the entrypoints to the device registration module.
 * This facility keeps track of active device nodes by interface type,
 * making it possible to find the set of devices of a particular type
 * simply be looking up all the registered interfaces with a particular IID.
 * For example, this allows you to find all the disk devices in the system,
 * all the SCSI disk devices, all Ethernet cards, etc.
 * The device registration module is implemented in libfdev by default,
 * but can be overridden by the OS to provide a registration mechanism
 * specialized to the OS's needs and/or integrated with existing mechanisms.
 */

/*
 * Register a set of device interfaces for a given device node.
 * The caller must provide a list of interface identifiers (IIDs)
 * for type-specific device interfaces it supports (e.g., oskit_blkdev_t)
 * in addition to oskit_device_t which all device nodes must support.
 */
oskit_error_t osenv_device_register(oskit_device_t *device,
				  const struct oskit_guid *iids,
				  unsigned iid_count);

/*
 * Unregister a device node registered with osenv_device_register().
 * The parameters passed here must be _exactly_ the same
 * as the ones that were passed to osenv_device_register() previously.
 */
oskit_error_t osenv_device_unregister(oskit_device_t *driver,
				    const struct oskit_guid *iids,
				    unsigned iid_count);

/*
 * Obtain a list of all the registered device interfaces with a specified IID.
 * The oskit_device_iid can be used to find _all_ registered devices,
 * and other IIDs can be used to find all devices of a specific type.
 * When the client is finished with the returned array,
 * it must release all the references it contains
 * and free the array itself using osenv_mem_free(arr, OSENV_AUTO_SIZE, 0).
 * Returns the number of interface pointers in the returned array;
 * if there are no matches in the database, returns 0
 * with *out_interface_array set to NULL.
 */
oskit_error_t osenv_device_lookup(const struct oskit_guid *iid,
				void ***out_interface_array);

/*
 * Initialize the osenv_device registration module. Should be called
 * from oskit_dev_init().
 */
void osenv_device_registration_init(void);

#endif /* _OSKIT_DEV_DEVICE_H_ */
