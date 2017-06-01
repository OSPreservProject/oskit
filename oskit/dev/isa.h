/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
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
 * Definitions of the oskit_isabus and oskit_isa_driver interfaces.
 */
#ifndef _OSKIT_DEV_ISA_H_
#define _OSKIT_DEV_ISA_H_

#include <oskit/dev/device.h>

struct oskit_membus;

/*
 * The ISA bus has a 16-bit I/O port address space (0-65535).
 * It also has a 20-bit memory space (16MB)
 */
typedef oskit_addr_t oskit_isa_port_t;


/*
 * ISA bus device interface,
 * IID 4aa7df84-7c74-11cf-b500-08000953adc2.
 * The oskit_isabus interface represents a node in a hardware tree
 * corresponding to an ISA bus.
 * Since an ISA bus is also a memory bus,
 * this interface extends the oskit_membus interface.
 *
 * This minimal ISA bus interface
 * is only sufficient to determine that the bus is an ISA bus
 * and to locate child devices on the bus;
 * direct access to the bus is supported
 * only if the object implements the oskit_isabus_io interface below
 * and the oskit_membus_io interface defined in <oskit/dev/membus.h>.
 */
struct oskit_isabus {
	struct oskit_isabus_ops *ops;
};
typedef struct oskit_isabus oskit_isabus_t;

struct oskit_isabus_ops {

	/* Inherited IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_isabus_t *bus,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_isabus_t *bus);
	OSKIT_COMDECL_U	(*release)(oskit_isabus_t *bus);

	/* Inherited oskit_device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_isabus_t *bus,
				    struct oskit_devinfo *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_isabus_t *bus,
				      struct oskit_driver **out_driver);

	/* Inherited oskit_bus interface operations */
	OSKIT_COMDECL	(*getchild)(oskit_isabus_t *bus, oskit_u32_t idx,
				     oskit_device_t **out_fdev, char *out_pos);
	OSKIT_COMDECL	(*probe)(oskit_isabus_t *bus);

	/* Inherited oskit_membus interface operations */

	/*
	 * Since most ISA devices are I/O-mapped rather than memory-mapped,
	 * this method will usually return 0 in *out_addr.
	 */
	OSKIT_COMDECL	(*getchildaddr)(oskit_isabus_t *bus, oskit_u32_t idx,
					oskit_device_t **out_fdev,
					oskit_addr_t *out_addr);

	/*** Operations specific to ISA buses ***/

	/*
	 * This is just like getchild(),
	 * except it returns the specified device's base address
	 * as a numeric I/O port rather than as a string.
	 *
	 * XXX what if the device is not I/O mapped?
	 * Does such a beast exist?
	 */
	OSKIT_COMDECL	(*getchildio)(oskit_isabus_t *bus, oskit_u32_t idx,
					 oskit_device_t **out_fdev,
					 oskit_u32_t *out_addr);
};

extern const struct oskit_guid oskit_isabus_iid;
#define OSKIT_ISABUS_IID OSKIT_GUID(0x4aa7df84, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_isabus_query(bus, iid, out_ihandle) \
	((bus)->ops->query((oskit_isabus_t *)(bus), (iid), (out_ihandle)))
#define oskit_isabus_addref(bus) \
	((bus)->ops->addref((oskit_isabus_t *)(bus)))
#define oskit_isabus_release(bus) \
	((bus)->ops->release((oskit_isabus_t *)(bus)))
#define oskit_isabus_getinfo(bus, out_info) \
	((bus)->ops->getinfo((oskit_isabus_t *)(bus), (out_info)))
#define oskit_isabus_getdriver(bus, out_driver) \
	((bus)->ops->getdriver((oskit_isabus_t *)(bus), (out_driver)))
#define oskit_isabus_getchild(bus, idx, out_fdev, out_pos) \
	((bus)->ops->getchild((oskit_isabus_t *)(bus), (idx), (out_fdev), (out_pos)))
#define oskit_isabus_probe(bus) \
	((bus)->ops->probe((oskit_isabus_t *)(bus)))
#define oskit_isabus_getchildaddr(bus, idx, out_fdev, out_addr) \
	((bus)->ops->getchildaddr((oskit_isabus_t *)(bus), (idx), (out_fdev), (out_addr)))
#define oskit_isabus_getchildio(bus, idx, out_fdev, out_addr) \
	((bus)->ops->getchildio((oskit_isabus_t *)(bus), (idx), (out_fdev), (out_addr)))


/*
 * Local ISA bus device interface,
 * IID 4aa7dfa3-7c74-11cf-b500-08000953adc2.
 * This interface is a subtype of oskit_isabus.
 * It does not define any additional methods,
 * but its presence implies that the bus's I/O space
 * can be accessed directly using local processor I/O instructions
 * without having to go through the I/O port access methods above.
 */
typedef oskit_isabus_t oskit_local_isabus_t;
extern const struct oskit_guid oskit_local_isabus_iid;
#define OSKIT_LOCAL_ISABUS_IID OSKIT_GUID(0x4aa7dfa3, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)


/*
 * ISA bus I/O interface,
 * IID 4aa7dfaa-7c74-11cf-b500-08000953adc2.
 * This interface extends the oskit_isabus interface,
 * providing additional methods allowing direct access to the bus.
 * In addition, objects supporting this interface
 * must also support the oskit_membus_io interface.
 */
struct oskit_isabus_io {
	struct oskit_isabus_io_ops *ops;
};
typedef struct oskit_isabus_io oskit_isabus_io_t;

struct oskit_isabus_io_ops {

	/* Inherited IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_isabus_io_t *bus,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_isabus_io_t *bus);
	OSKIT_COMDECL_U	(*release)(oskit_isabus_io_t *bus);

	/* Inherited oskit_device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_isabus_io_t *bus,
				    struct oskit_devinfo *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_isabus_io_t *bus,
				      struct oskit_driver **out_driver);

	/* Inherited oskit_bus interface operations */
	OSKIT_COMDECL	(*getchild)(oskit_isabus_io_t *bus, oskit_u32_t idx,
				     oskit_device_t **out_fdev, char *out_pos);
	OSKIT_COMDECL	(*probe)(oskit_isabus_io_t *bus);

	/* Inherited oskit_membus interface operations */
	OSKIT_COMDECL	(*getchildaddr)(oskit_isabus_io_t *bus, oskit_u32_t idx,
					oskit_device_t **out_fdev,
					oskit_addr_t *out_addr);

	/* Inherited oskit_isabus interface operations */
	OSKIT_COMDECL	(*getchildio)(oskit_isabus_t *bus, oskit_u32_t idx,
					 oskit_device_t **out_fdev,
					 oskit_u32_t *out_addr);

	/*** Operations specific to ISA buses ***/

	/*
	 * Device drivers for devices that can exist on this bus
	 * call this method to register themselves
	 * after having found and initialized a device on this bus,
	 * or to unregister themselves if the device is removed.
	 * The 'addr' parameter specified to each of these methods must be
	 * the base I/O port address at which this device appears.
	 * If the device uses multiple I/O address ranges,
	 * then the device's "primary" I/O address should be used.
	 *
	 * XXX what if the device is not I/O mapped?
	 * Does such a beast exist?
	 */
	OSKIT_COMDECL	(*addchild)(oskit_isabus_io_t *bus, oskit_u32_t addr,
				     oskit_device_t *dev);
	OSKIT_COMDECL	(*remchild)(oskit_isabus_io_t *bus, oskit_u32_t addr);

	/*** I/O Port Management ***/

	/* Returns true if the specified range of I/O ports is available. */
	oskit_bool_t
	  OSKIT_COMCALL	(*port_avail)(oskit_isabus_io_t *bus,
				      oskit_isa_port_t port, oskit_u32_t size);

	/*
	 * Allocate a range of ports in the ISA I/O space.
	 * Each range has an associated owner device for bookkeeping purposes;
	 * the allocating device must pass a reference to its own device node.
	 * The port_alloc() method returns OSKIT_E_DEV_SPACE_INUSE
	 * if any of the requested ports are already allocated.
	 */
	OSKIT_COMDECL	(*port_alloc)(oskit_isabus_io_t *bus,
				      oskit_isa_port_t port, oskit_u32_t size,
				      oskit_device_t *owner);

	/*
	 * Free a previously allocated port range.
	 * The 'port' and 'size' must be _exactly_ as specified to port_alloc():
	 * do not attempt to free part of a previously allocated range,
	 * or two consecutive ranges allocated previously, etc.
	 */
	OSKIT_COMDECL	(*port_free)(oskit_isabus_io_t *bus,
				     oskit_isa_port_t port, oskit_u32_t size);

	/*
	 * Scan the I/O port space for the first allocated I/O port
	 * greater than or equal to the initial value in *inout_port,
	 * and return the actual port address, size, and owner of the range.
	 * Returns OSKIT_S_FALSE if no more allocated ranges exist.
	 */
	OSKIT_COMDECL	(*port_scan)(oskit_isabus_io_t *bus,
				     oskit_isa_port_t *inout_port,
				     oskit_u32_t *out_size,
				     oskit_device_t **out_owner);


	/*** I/O Port Access ***/

	/* Read a byte, 16-bit word, or 32-bit longword from ISA I/O space */
	oskit_u8_t
	  OSKIT_COMCALL	(*inb)(oskit_isabus_io_t *bus, oskit_isa_port_t port);
	oskit_u16_t
	  OSKIT_COMCALL	(*inw)(oskit_isabus_io_t *bus, oskit_isa_port_t port);
	oskit_u32_t
	  OSKIT_COMCALL	(*inl)(oskit_isabus_io_t *bus, oskit_isa_port_t port);

	/* Write a byte, 16-bit word, or 32-bit longword to ISA I/O space */
	OSKIT_COMDECL_V	(*outb)(oskit_isabus_io_t *bus, oskit_isa_port_t port,
				oskit_u8_t val);
	OSKIT_COMDECL_V	(*outw)(oskit_isabus_io_t *bus, oskit_isa_port_t port,
				oskit_u16_t val);
	OSKIT_COMDECL_V	(*outl)(oskit_isabus_io_t *bus, oskit_isa_port_t port,
				oskit_u32_t val);

	/* Read strings of data from I/O space */
	OSKIT_COMDECL_V	(*insb)(oskit_isabus_io_t *bus, oskit_isa_port_t port,
				oskit_u8_t *buf, oskit_u32_t count);
	OSKIT_COMDECL_V	(*insw)(oskit_isabus_io_t *bus, oskit_isa_port_t port,
				oskit_u16_t *buf, oskit_u32_t count);
	OSKIT_COMDECL_V	(*insl)(oskit_isabus_io_t *bus, oskit_isa_port_t port,
				oskit_u32_t *buf, oskit_u32_t count);

	/* Write strings of data to I/O space */
	OSKIT_COMDECL_V	(*outsb)(oskit_isabus_io_t *bus, oskit_isa_port_t port,
				 const oskit_u8_t *buf, oskit_u32_t count);
	OSKIT_COMDECL_V	(*outsw)(oskit_isabus_io_t *bus, oskit_isa_port_t port,
				 const oskit_u16_t *buf, oskit_u32_t count);
	OSKIT_COMDECL_V	(*outsl)(oskit_isabus_io_t *bus, oskit_isa_port_t port,
				 const oskit_u32_t *buf, oskit_u32_t count);
};

extern const struct oskit_guid oskit_isabus_io_iid;
#define OSKIT_ISABUS_IO_IID OSKIT_GUID(0x4aa7dfaa, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_isabus_io_query(bus, iid, out_ihandle) \
	((bus)->ops->query((oskit_isabus_io_t *)(bus), (iid), (out_ihandle)))
#define oskit_isabus_io_addref(bus) \
	((bus)->ops->addref((oskit_isabus_io_t *)(bus)))
#define oskit_isabus_io_release(bus) \
	((bus)->ops->release((oskit_isabus_io_t *)(bus)))
#define oskit_isabus_io_getinfo(bus, out_info) \
	((bus)->ops->getinfo((oskit_isabus_io_t *)(bus), (out_info)))
#define oskit_isabus_io_getdriver(bus, out_driver) \
	((bus)->ops->getdriver((oskit_isabus_io_t *)(bus), (out_driver)))
#define oskit_isabus_io_getchild(bus, idx, out_fdev, out_pos) \
	((bus)->ops->getchild((oskit_isabus_io_t *)(bus), (idx), (out_fdev), (out_pos)))
#define oskit_isabus_io_probe(bus) \
	((bus)->ops->probe((oskit_isabus_io_t *)(bus)))
#define oskit_isabus_io_getchildaddr(bus, idx, out_fdev, out_addr) \
	((bus)->ops->getchildaddr((oskit_isabus_io_t *)(bus), (idx), (out_fdev), (out_addr)))
#define oskit_isabus_io_getchildio(bus, idx, out_fdev, out_addr) \
	((bus)->ops->getchildio((oskit_isabus_io_t *)(bus), (idx), (out_fdev), (out_addr)))
#define oskit_isabus_io_addchild(bus, addr, dev) \
	((bus)->ops->addchild((oskit_isabus_io_t *)(bus), (addr), (dev)))
#define oskit_isabus_io_remchild(bus, addr) \
	((bus)->ops->remchild((oskit_isabus_io_t *)(bus), (addr)))
#define oskit_isabus_io_port_avail(bus, port, size) \
	((bus)->ops->port_avail((oskit_isabus_io_t *)(bus), (port), (size)))
#define oskit_isabus_io_port_alloc(bus, port, size, owner) \
	((bus)->ops->port_alloc((oskit_isabus_io_t *)(bus), (port), (size), (owner)))
#define oskit_isabus_io_port_free(bus, port, size) \
	((bus)->ops->port_free((oskit_isabus_io_t *)(bus), (port), (size)))
#define oskit_isabus_io_port_scan(bus, inout_port, out_size, out_owner) \
	((bus)->ops->port_scan((oskit_isabus_io_t *)(bus), (inout_port), (out_size), (out_owner)))
#define oskit_isabus_io_inb(bus, port) \
	((bus)->ops->inb((oskit_isabus_io_t *)(bus), (port)))
#define oskit_isabus_io_inw(bus, port) \
	((bus)->ops->inw((oskit_isabus_io_t *)(bus), (port)))
#define oskit_isabus_io_inl(bus, port) \
	((bus)->ops->inl((oskit_isabus_io_t *)(bus), (port)))
#define oskit_isabus_io_outb(bus, port, val) \
	((bus)->ops->outb((oskit_isabus_io_t *)(bus), (port), (val)))
#define oskit_isabus_io_outw(bus, port, val) \
	((bus)->ops->outw((oskit_isabus_io_t *)(bus), (port), (val)))
#define oskit_isabus_io_outl(bus, port, val) \
	((bus)->ops->outl((oskit_isabus_io_t *)(bus), (port), (val)))
#define oskit_isabus_io_insb(bus, port, buf, count) \
	((bus)->ops->insb((oskit_isabus_io_t *)(bus), (port), (buf), (count)))
#define oskit_isabus_io_insw(bus, port, buf, count) \
	((bus)->ops->insw((oskit_isabus_io_t *)(bus), (port), (buf), (count)))
#define oskit_isabus_io_insl(bus, port, buf, count) \
	((bus)->ops->insl((oskit_isabus_io_t *)(bus), (port), (buf), (count)))
#define oskit_isabus_io_outsb(bus, port, buf, count) \
	((bus)->ops->outsb((oskit_isabus_io_t *)(bus), (port), (buf), (count)))
#define oskit_isabus_io_outsw(bus, port, buf, count) \
	((bus)->ops->outsw((oskit_isabus_io_t *)(bus), (port), (buf), (count)))
#define oskit_isabus_io_outsl(bus, port, buf, count) \
	((bus)->ops->outsl((oskit_isabus_io_t *)(bus), (port), (buf), (count)))


/*
 * Interface to an ISA bus device driver,
 * IID 4aa7df85-7c74-11cf-b500-08000953adc2.
 * The oskit_isa_driver interface is exported by any device driver
 * that supports an ISA bus device of some kind;
 * it is through this interface that the driver is initialized
 * and instructed to probe the ISA bus for the device(s) it supports.
 *
 * XXX define driver priority based on lockup frequency etc.
 */
struct oskit_isa_driver {
	struct oskit_isa_driver_ops *ops;
};
typedef struct oskit_isa_driver oskit_isa_driver_t;

struct oskit_isa_driver_ops {

	/* Inherited IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_isa_driver_t *drv,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_isa_driver_t *drv);
	OSKIT_COMDECL_U	(*release)(oskit_isa_driver_t *drv);

	/* Inherited oskit_driver operations */
	OSKIT_COMDECL	(*getinfo)(oskit_isa_driver_t *drv,
				    oskit_devinfo_t *out_info);

	/*** Operations specific to ISA device drivers ***/

	/*
	 * Probe for devices on the ISA bus,
	 * creating new device nodes as appropriate
	 * and attaching them to the specified ISA bus device node.
	 * On success, returns the number of devices found and attached,
	 * 0 if everything went OK but nothing was found.
	 */
	OSKIT_COMDECL	(*probe)(oskit_isa_driver_t *drv,
				 oskit_isabus_t *bus);
};

extern const struct oskit_guid oskit_isa_driver_iid;
#define OSKIT_ISA_DRIVER_IID OSKIT_GUID(0x4aa7df85, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_isa_driver_query(drv, iid, out_ihandle) \
	((drv)->ops->query((oskit_isa_driver_t *)(drv), (iid), (out_ihandle)))
#define oskit_isa_driver_addref(drv) \
	((drv)->ops->addref((oskit_isa_driver_t *)(drv)))
#define oskit_isa_driver_release(drv) \
	((drv)->ops->release((oskit_isa_driver_t *)(drv)))
#define oskit_isa_driver_getinfo(drv, out_info) \
	((drv)->ops->getinfo((oskit_isa_driver_t *)(drv), (out_info)))
#define oskit_isa_driver_probe(drv, bus) \
	((drv)->ops->probe((oskit_isa_driver_t *)(drv), (bus)))

#endif /* _OSKIT_DEV_ISA_H_ */
