/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * OSKit device driver interface definitions.
 */
#ifndef _OSKIT_DEV_BUS_H_
#define _OSKIT_DEV_BUS_H_

#include <oskit/com.h>
#include <oskit/dev/error.h>

struct oskit_guid;
struct oskit_devinfo;
struct oskit_driver;
struct oskit_device;

/*
 * Generic bus device node interface,
 * IID 4aa7df83-7c74-11cf-b500-08000953adc2.
 * A bus is a device node representing a device
 * to which other "child" devices can be attached.
 */
struct oskit_bus
{
	struct oskit_bus_ops *ops;
};
typedef struct oskit_bus oskit_bus_t;

struct oskit_bus_ops
{
	/* Inherited IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_bus_t *bus, const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_bus_t *bus);
	OSKIT_COMDECL_U	(*release)(oskit_bus_t *bus);

	/* Inherited oskit_device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_bus_t *bus, 
				    struct oskit_devinfo *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_bus_t *fdev,
				      struct oskit_driver **out_driver);

	/*** Bus device-specific operations ***/

	/*
	 * Returns the 'idx'th child of this bus,
	 * or returns OSKIT_E_DEV_NOMORE_CHILDREN if 'idx' is too large.
	 * Children have consecutive indices starting from zero;
	 * the order in which children appear is defined by the driver.
	 * To enumerate all the children of a particular bus,
	 * simply call this function with successive indices
	 * until it returns OSKIT_E_DEV_NOMORE_CHILDREN.
	 * For each child, this function returns the child's device node,
	 * and a string at most OSKIT_BUS_POS_MAX characters long
	 * in a (hopefully) more useful and stable textual form:
	 * e.g., "io320" for I/O address 0x320 on an ISA bus.
	 */
	OSKIT_COMDECL	(*getchild)(oskit_bus_t *bus, unsigned idx,
				     struct oskit_device **out_fdev,
				     char *out_pos);

	/*
	 * Probe for devices on this bus,
	 * typically by searching for and calling appropriate drivers
	 * registered in the driver registration database (see register.h).
	 * If successful, returns the number of new devices found,
	 * or 0 if no error occurred but nothing new was found.
	 */
	OSKIT_COMDECL	(*probe)(oskit_bus_t *bus);
};

#define oskit_bus_query(bus, iid, out_ihandle) \
	((bus)->ops->query((oskit_bus_t *)(bus), (iid), (out_ihandle)))
#define oskit_bus_addref(bus) \
	((bus)->ops->addref((oskit_bus_t *)(bus)))
#define oskit_bus_release(bus) \
	((bus)->ops->release((oskit_bus_t *)(bus)))
#define oskit_bus_getinfo(bus, out_info) \
	((bus)->ops->getinfo((oskit_bus_t *)(bus), (out_info)))
#define oskit_bus_getdriver(fdev, out_driver) \
	((fdev)->ops->getdriver((oskit_bus_t *)(fdev), (out_driver)))
#define oskit_bus_getchild(bus, idx, out_fdev, out_pos) \
	((bus)->ops->getchild((oskit_bus_t *)(bus), (idx), (out_fdev), (out_pos)))
#define oskit_bus_probe(bus) \
	((bus)->ops->probe((oskit_bus_t *)(bus)))

extern const struct oskit_guid oskit_bus_iid;
#define OSKIT_BUS_IID OSKIT_GUID(0x4aa7df83, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

/*
 * Maximum number of characters in bus position string,
 * not including the terminating null.
 * Must be greater than 16 to allow for 64-bit addresses in hex.
 */
#define OSKIT_BUS_POS_MAX	63

#endif /* _OSKIT_DEV_BUS_H_ */
