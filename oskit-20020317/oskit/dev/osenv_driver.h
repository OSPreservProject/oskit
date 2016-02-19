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
 * COMification of the osenv driver registration interface. 
 */
#ifndef _OSKIT_DEV_OSENV_DRIVER_H_
#define _OSKIT_DEV_OSENV_DRIVER_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/driver.h>

/*
 * osenv driver registration interface.
 *
 * IID 4aa7dff0-7c74-11cf-b500-08000953adc2
 */
struct oskit_osenv_driver {
	struct oskit_osenv_driver_ops *ops;
};
typedef struct oskit_osenv_driver oskit_osenv_driver_t;

struct oskit_osenv_driver_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_osenv_driver_t)

	OSKIT_COMDECL   (*Register)(oskit_osenv_driver_t *o,
				struct oskit_driver *driver,
				const struct oskit_guid *iids,
				unsigned iid_count);

	OSKIT_COMDECL   (*unregister)(oskit_osenv_driver_t *o,
				struct oskit_driver *driver,
				const struct oskit_guid *iids,
				unsigned iid_count);

	OSKIT_COMDECL   (*lookup)(oskit_osenv_driver_t *o,
				const struct oskit_guid *iid,
				void ***out_interface_array);
};

/* GUID for oskit_osenv_driver interface */
extern const struct oskit_guid oskit_osenv_driver_iid;
#define OSKIT_OSENV_DRIVER_IID OSKIT_GUID(0x4aa7dff0, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_osenv_driver_query(o, iid, out_ihandle) \
	((o)->ops->query((oskit_osenv_driver_t *)(o), (iid), (out_ihandle)))
#define oskit_osenv_driver_addref(o) \
	((o)->ops->addref((oskit_osenv_driver_t *)(o)))
#define oskit_osenv_driver_release(o) \
	((o)->ops->release((oskit_osenv_driver_t *)(o)))

#define oskit_osenv_driver_register(o, dev, iids, iid_count) \
	((o)->ops->Register((oskit_osenv_driver_t *)(o), \
				   (dev), (iids), (iid_count)))
#define oskit_osenv_driver_unregister(o, dev, iids, iid_count) \
	((o)->ops->unregister((oskit_osenv_driver_t *)(o), \
				     (dev), (iids), (iid_count)))
#define oskit_osenv_driver_lookup(o, iid, out_array) \
	((o)->ops->lookup((oskit_osenv_driver_t *)(o), (iid), (out_array)))

/*
 * Return a reference to an osenv driver registration interface object.
 */
oskit_osenv_driver_t	*oskit_create_osenv_driver(void);

#endif /* _OSKIT_DEV_OSENV_DRIVER_H_ */
