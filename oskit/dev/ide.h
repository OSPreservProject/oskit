/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * OSKit device driver interface definitions for IDE adaptors.
 */
#ifndef _OSKIT_DEV_IDE_H_
#define _OSKIT_DEV_IDE_H_

#include <oskit/com.h>

struct oskit_devinfo;
struct oskit_driver;
struct oskit_device;

/*
 * IDE bus device node interface,
 * IID 4aa7df92-7c74-11cf-b500-08000953adc2.
 * An IDE bus can have at most two child devices attached:
 * one master (position 0) and one slave (position 1).
 */
struct oskit_idebus
{
	struct oskit_idebus_ops *ops;
};
typedef struct oskit_idebus oskit_idebus_t;

struct oskit_idebus_ops
{
	/* Inherited IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_idebus_t *bus,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_idebus_t *bus);
	OSKIT_COMDECL_U	(*release)(oskit_idebus_t *bus);

	/* Inherited oskit_device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_idebus_t *bus, 
				    struct oskit_devinfo *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_idebus_t *bus,
				      struct oskit_driver **out_driver);

	/*** Inherited oskit_bus interface operations */
	OSKIT_COMDECL	(*getchild)(oskit_idebus_t *bus, oskit_u32_t idx,
				     struct oskit_device **out_fdev,
				     char *out_pos);
	OSKIT_COMDECL	(*probe)(oskit_idebus_t *bus);

	/*** Methods specific to IDE buses ***/

	/*
	 * This method is just like getchild,
	 * except it returns the child's bus position as a numeric value:
	 * 0 = master, 1 = slave.
	 */
	OSKIT_COMDECL	(*getchildpos)(oskit_idebus_t *bus, oskit_u32_t idx,
				       struct oskit_device **out_fdev,
				       oskit_u32_t *out_pos);
};

#define OSKIT_idebus_query(bus, iid, out_ihandle) \
	((bus)->ops->query((oskit_idebus_t *)(bus), (iid), (out_ihandle)))
#define oskit_idebus_addref(bus) \
	((bus)->ops->addref((oskit_idebus_t *)(bus)))
#define oskit_idebus_release(bus) \
	((bus)->ops->release((oskit_idebus_t *)(bus)))
#define oskit_idebus_getinfo(bus, out_info) \
	((bus)->ops->getinfo((oskit_idebus_t *)(bus), (out_info)))
#define oskit_idebus_getdriver(bus, out_driver) \
	((bus)->ops->getdriver((oskit_idebus_t *)(bus), (out_driver)))
#define oskit_idebus_getchild(bus, idx, out_fdev, out_pos) \
	((bus)->ops->getchild((oskit_idebus_t *)(bus), (idx), (out_fdev), (out_pos)))
#define oskit_idebus_probe(bus) \
	((bus)->ops->probe((oskit_idebus_t *)(bus)))
#define oskit_idebus_getchildpos(bus, idx, out_fdev, out_pos) \
	((bus)->ops->getchildpos((oskit_idebus_t *)(bus), (idx), (out_fdev), (out_pos)))

extern const struct oskit_guid oskit_idebus_iid;
#define OSKIT_IDEBUS_IID OSKIT_GUID(0x4aa7df92, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)


#if 0	/* XXX this interface is not yet complete or even functional. */
/*
 * IDE bus I/O interface,
 * IID 4aa7dfac-7c74-11cf-b500-08000953adc2.
 */
struct oskit_idebus_io
{
	struct oskit_idebus_io_ops *ops;
};
typedef struct oskit_idebus_io oskit_idebus_io_t;

struct oskit_idebus_io_ops
{
	/* Inherited IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_idebus_io_t *bus,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_idebus_io_t *bus);
	OSKIT_COMDECL_U	(*release)(oskit_idebus_io_t *bus);

	/* Inherited oskit_device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_idebus_io_t *bus, 
				    struct oskit_devinfo *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_idebus_io_t *bus,
				      struct oskit_driver **out_driver);

	/*** Inherited oskit_bus interface operations */
	OSKIT_COMDECL	(*getchild)(oskit_idebus_io_t *bus, oskit_u32_t idx,
				     struct oskit_device **out_fdev,
				     char *out_pos);
	OSKIT_COMDECL	(*probe)(oskit_idebus_io_t *bus);

	/*** Inherited oskit_idebus interface operations */
	OSKIT_COMDECL	(*getchildpos)(oskit_idebus_t *bus, oskit_u32_t idx,
				       struct oskit_device **out_fdev,
				       oskit_u32_t *out_pos);

	/*** Methods specific to IDE buses ***/

	/*
	 * Add or remove a child device on this IDE bus.
	 * The position specified must be either 0 or 1,
	 * indicating master or slave, respectively.
	 */
	OSKIT_COMDECL	(*addchild)(oskit_idebus_io_t *bus, oskit_u32_t pos,
				     struct oskit_device *dev);
	OSKIT_COMDECL	(*remchild)(oskit_idebus_io_t *bus, oskit_u32_t pos);

	XXX add appropriate direct access methods
};

/* XXX extern const struct oskit_guid oskit_idebus_io_iid; */
#define OSKIT_IDEBUS_IO_IID OSKIT_GUID(0x4aa7dfac, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif /* 0 */


#endif /* _OSKIT_DEV_IDE_H_ */
