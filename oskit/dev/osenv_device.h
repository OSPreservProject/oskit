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
/*
 * COMification of the osenv device registration interface. 
 */
#ifndef _OSKIT_DEV_OSENV_DEVICE_H_
#define _OSKIT_DEV_OSENV_DEVICE_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/device.h>

/*
 * osenv device registration interface.
 *
 * IID 4aa7dfef-7c74-11cf-b500-08000953adc2
 */
struct oskit_osenv_device {
	struct oskit_osenv_device_ops *ops;
};
typedef struct oskit_osenv_device oskit_osenv_device_t;

struct oskit_osenv_device_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_osenv_device_t)

	OSKIT_COMDECL   (*Register)(oskit_osenv_device_t *o,
				struct oskit_device *device,
				const struct oskit_guid *iids,
				unsigned iid_count);

	OSKIT_COMDECL   (*unregister)(oskit_osenv_device_t *o,
				struct oskit_device *device,
				const struct oskit_guid *iids,
				unsigned iid_count);

	OSKIT_COMDECL   (*lookup)(oskit_osenv_device_t *o,
				const struct oskit_guid *iid,
				void ***out_interface_array);
};

/* GUID for oskit_osenv_device interface */
extern const struct oskit_guid oskit_osenv_device_iid;
#define OSKIT_OSENV_DEVICE_IID OSKIT_GUID(0x4aa7dfef, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_osenv_device_query(o, iid, out_ihandle) \
	((o)->ops->query((oskit_osenv_device_t *)(o), (iid), (out_ihandle)))
#define oskit_osenv_device_addref(o) \
	((o)->ops->addref((oskit_osenv_device_t *)(o)))
#define oskit_osenv_device_release(o) \
	((o)->ops->release((oskit_osenv_device_t *)(o)))

#define oskit_osenv_device_register(o, dev, iids, iid_count) \
	((o)->ops->Register((oskit_osenv_device_t *)(o), \
				   (dev), (iids), (iid_count)))
#define oskit_osenv_device_unregister(o, dev, iids, iid_count) \
	((o)->ops->unregister((oskit_osenv_device_t *)(o), \
				     (dev), (iids), (iid_count)))
#define oskit_osenv_device_lookup(o, iid, out_array) \
	((o)->ops->lookup((oskit_osenv_device_t *)(o), (iid), (out_array)))

/*
 * Return a reference to an osenv device registration interface object.
 */
oskit_osenv_device_t	*oskit_create_osenv_device(void);

#endif /* _OSKIT_DEV_OSENV_DEVICE_H_ */

