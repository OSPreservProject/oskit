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
 * OSKit device driver interface definitions for SCSI host adaptors.
 */
#ifndef _OSKIT_DEV_SCSI_H_
#define _OSKIT_DEV_SCSI_H_

#include <oskit/com.h>
#include <oskit/dev/error.h>

struct oskit_devinfo;
struct oskit_driver;
struct oskit_device;

/*
 * SCSI bus (host adaptor) device node interface,
 * IID 4aa7df97-7c74-11cf-b500-08000953adc2.
 * This minimal SCSI bus interface
 * is only sufficient to determine the bus's type
 * and to locate child devices on the bus;
 * direct access to the SCSI bus is supported
 * only if the object implements the oskit_scsibus_io interface below.
 */
struct oskit_scsibus
{
	struct oskit_scsibus_ops *ops;
};
typedef struct oskit_scsibus oskit_scsibus_t;

struct oskit_scsibus_ops
{
	/* Inherited IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_scsibus_t *bus,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_scsibus_t *bus);
	OSKIT_COMDECL_U	(*release)(oskit_scsibus_t *bus);

	/* Inherited oskit_device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_scsibus_t *bus, 
				    struct oskit_devinfo *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_scsibus_t *bus,
				      struct oskit_driver **out_driver);

	/*** Inherited oskit_bus interface operations */
	OSKIT_COMDECL	(*getchild)(oskit_scsibus_t *bus, oskit_u32_t idx,
				     struct oskit_device **out_fdev,
				     char *out_pos);
	OSKIT_COMDECL	(*probe)(oskit_scsibus_t *bus);

	/*** Methods specific to SCSI buses ***/

	/*
	 * This method is just like the getchild method,
	 * except it returns the child device's position
	 * in terms of the device's SCSI bus ID and logical unit number (LUN).
	 */
	OSKIT_COMDECL	(*getchildid)(oskit_scsibus_t *bus, oskit_u32_t idx,
				      struct oskit_device **out_fdev,
				      oskit_u32_t *out_id, oskit_u32_t *out_lun);
};

#define oskit_scsibus_query(bus, iid, out_ihandle) \
	((bus)->ops->query((oskit_scsibus_t *)(bus), (iid), (out_ihandle)))
#define oskit_scsibus_addref(bus) \
	((bus)->ops->addref((oskit_scsibus_t *)(bus)))
#define oskit_scsibus_release(bus) \
	((bus)->ops->release((oskit_scsibus_t *)(bus)))
#define oskit_scsibus_getinfo(bus, out_info) \
	((bus)->ops->getinfo((oskit_scsibus_t *)(bus), (out_info)))
#define oskit_scsibus_getdriver(bus, out_driver) \
	((bus)->ops->getdriver((oskit_scsibus_t *)(bus), (out_driver)))
#define oskit_scsibus_getchild(bus, idx, out_fdev, out_pos) \
	((bus)->ops->getchild((oskit_scsibus_t *)(bus), (idx), (out_fdev), (out_pos)))
#define oskit_scsibus_probe(bus) \
	((bus)->ops->probe((oskit_scsibus_t *)(bus)))
#define oskit_scsibus_getchildid(bus, idx, out_fdev, out_id, out_lun) \
	((bus)->ops->getchildid((oskit_scsibus_t *)(bus), (idx), (out_fdev), (out_id), (out_lun)))

extern const struct oskit_guid oskit_scsibus_iid;
#define OSKIT_SCSIBUS_IID OSKIT_GUID(0x4aa7df97, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)


#if 0 /* XXX this interface is not yet complete or even functional */
/*
 * SCSI bus (host adaptor) device node interface,
 * IID 4aa7dfab-7c74-11cf-b500-08000953adc2.
 * This minimal SCSI bus interface
 * is only sufficient to determine the bus's type
 * and to locate child devices on the bus;
 * direct access to the SCSI bus is supported
 * only if the object implements the oskit_scsibus_io interface below.
 */
struct oskit_scsibus_io
{
	struct oskit_scsibus_io_ops *ops;
};
typedef struct oskit_scsibus_io oskit_scsibus_io_t;

struct oskit_scsibus_io_ops
{
	/* Inherited IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_scsibus_io_t *bus,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_scsibus_io_t *bus);
	OSKIT_COMDECL_U	(*release)(oskit_scsibus_io_t *bus);

	/* Inherited oskit_device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_scsibus_io_t *bus, 
				    struct oskit_devinfo *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_scsibus_io_t *bus,
				      struct oskit_driver **out_driver);

	/*** Inherited oskit_bus interface operations */
	OSKIT_COMDECL	(*getchild)(oskit_scsibus_io_t *bus, unsigned idx,
				     struct oskit_device **out_fdev,
				     char *out_pos);
	OSKIT_COMDECL	(*probe)(oskit_scsibus_io_t *bus);

	/*** Inherited oskit_scsibus_io interface operations */
	OSKIT_COMDECL	(*getchildid)(oskit_scsibus_t *bus, unsigned idx,
				      struct oskit_device **out_fdev,
				      oskit_u32_t *out_id, oskit_u32_t *out_lun);

	/*** Methods specific to SCSI I/O ***/

	/*
	 * Add or remove a child device on this SCSI bus.
	 */
	OSKIT_COMDECL	(*addchild)(oskit_scsibus_io_t *bus,
				    oskit_u32_t id, oskit_u32_t lun,
				    struct oskit_device *dev);
	OSKIT_COMDECL	(*remchild)(oskit_scsibus_io_t *bus,
				    unsigned id, unsigned lun);

	XXX operation to find host adaptors own SCSI ID
	XXX operations to send SCSI commands etc.
};

/* XXX extern const struct oskit_guid oskit_scsibus_io_iid; */
#define OSKIT_SCSIBUS_IO_IID OSKIT_GUID(0x4aa7dfab, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)
#endif /* 0 */

#endif /* _OSKIT_DEV_SCSI_H_ */
