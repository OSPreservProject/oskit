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
 * COMification of the osenv ioport interface. 
 */
#ifndef _OSKIT_DEV_OSENV_IOPORT_H_
#define _OSKIT_DEV_OSENV_IOPORT_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/dev/dev.h>

/*
 * osenv ioport interface.
 *
 * IID 4aa7dff4-7c74-11cf-b500-08000953adc2
 */
struct oskit_osenv_ioport {
	struct oskit_osenv_ioport_ops *ops;
};
typedef struct oskit_osenv_ioport oskit_osenv_ioport_t;

struct oskit_osenv_ioport_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_osenv_ioport_t)

	OSKIT_COMDECL_U (*avail)(oskit_osenv_ioport_t *o,
				oskit_addr_t port, oskit_size_t size);
	OSKIT_COMDECL   (*alloc)(oskit_osenv_ioport_t *o,
				oskit_addr_t port, oskit_size_t size);
	OSKIT_COMDECL_V (*free)(oskit_osenv_ioport_t *o,
				oskit_addr_t port, oskit_size_t size);
};

/* GUID for oskit_osenv_ioport interface */
extern const struct oskit_guid oskit_osenv_ioport_iid;
#define OSKIT_OSENV_IOPORT_IID OSKIT_GUID(0x4aa7dff4, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_osenv_ioport_query(s, iid, out_ihandle) \
	((s)->ops->query((oskit_osenv_ioport_t *)(s), (iid), (out_ihandle)))
#define oskit_osenv_ioport_addref(s) \
	((s)->ops->addref((oskit_osenv_ioport_t *)(s)))
#define oskit_osenv_ioport_release(s) \
	((s)->ops->release((oskit_osenv_ioport_t *)(s)))

#define oskit_osenv_ioport_avail(s, port, size) \
	((s)->ops->avail((oskit_osenv_ioport_t *)(s), (port), (size)))
#define oskit_osenv_ioport_alloc(s, port, size) \
	((s)->ops->alloc((oskit_osenv_ioport_t *)(s), (port), (size)))
#define oskit_osenv_ioport_free(s, port, size) \
	((s)->ops->free((oskit_osenv_ioport_t *)(s), (port), (size)))

/*
 * Return a reference to an osenv ioport interface object.
 */
oskit_osenv_ioport_t	*oskit_create_osenv_ioport(void);

#endif /* _OSKIT_DEV_OSENV_IOPORT_H_ */

