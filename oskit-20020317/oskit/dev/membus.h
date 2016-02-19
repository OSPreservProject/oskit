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
 * Definitions of the oskit_membus and oskit_membus_driver interfaces,
 * used to manage buses that provide a memory access interface
 * (e.g., ISA, EISA, PCI, etc.; but not SCSI, IDE, USB, etc.).
 */
#ifndef _OSKIT_DEV_MEMBUS_H_
#define _OSKIT_DEV_MEMBUS_H_

#include <oskit/dev/error.h>
#include <oskit/dev/device.h>

/*
 * Generic memory bus device interface,
 * IID 4aa7df89-7c74-11cf-b500-08000953adc2.
 * The oskit_membus interface represents a node in a hardware tree
 * corresponding to a bus presenting a memory-like address space,
 * which can be mapped into the processor's virtual memory space.
 *
 * This minimal memory bus interface
 * is only sufficient to determine the bus's type
 * and to locate child devices on the bus;
 * direct access to the bus is supported
 * only if the object implements the oskit_membus_io interface below.
 *
 * XXX should the addresses used in this interface
 * be consistently 32-bit or 64-bit, rather than using oskit_addr_t?
 */
struct oskit_membus {
	struct oskit_membus_ops *ops;
};
typedef struct oskit_membus oskit_membus_t;

struct oskit_membus_ops {

	/* Inherited IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_membus_t *bus,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_membus_t *bus);
	OSKIT_COMDECL_U	(*release)(oskit_membus_t *bus);

	/* Inherited oskit_device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_membus_t *bus, oskit_devinfo_t *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_membus_t *fdev,
				      oskit_driver_t **out_driver);

	/* Inherited oskit_bus interface operations */
	OSKIT_COMDECL	(*getchild)(oskit_membus_t *bus, oskit_u32_t idx,
				     oskit_device_t **out_fdev, char *out_pos);
	OSKIT_COMDECL	(*probe)(oskit_membus_t *bus);

	/*** Operations specific to memory buses ***/

	/*
	 * This is just like getchild(),
	 * except it returns the specified device's base address
	 * as a numeric physical memory address rather than as a string.
	 * If the device is not memory-mapped,
	 * then *out_addr will be set to zero on return.
	 */
	OSKIT_COMDECL	(*getchildaddr)(oskit_membus_t *bus, oskit_u32_t idx,
					oskit_device_t **out_fdev,
					oskit_addr_t *out_addr);
};

extern const struct oskit_guid oskit_membus_iid;
#define OSKIT_MEMBUS_IID OSKIT_GUID(0x4aa7df89, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_membus_query(bus, iid, out_ihandle) \
	((bus)->ops->query((oskit_membus_t *)(bus), (iid), (out_ihandle)))
#define oskit_membus_addref(bus) \
	((bus)->ops->addref((oskit_membus_t *)(bus)))
#define oskit_membus_release(bus) \
	((bus)->ops->release((oskit_membus_t *)(bus)))
#define oskit_membus_getinfo(bus, out_info) \
	((bus)->ops->getinfo((oskit_membus_t *)(bus), (out_info)))
#define oskit_membus_getdriver(fdev, out_driver) \
	((fdev)->ops->getdriver((oskit_membus_t *)(fdev), (out_driver)))
#define oskit_membus_getchild(bus, idx, out_fdev, out_pos) \
	((bus)->ops->getchild((oskit_membus_t *)(bus), (idx), (out_fdev), (out_pos)))
#define oskit_membus_probe(bus) \
	((bus)->ops->probe((oskit_membus_t *)(bus)))
#define oskit_membus_getchildaddr(bus, idx, out_fdev, out_addr) \
	((bus)->ops->getchildaddr((oskit_membus_t *)(bus), (idx), (out_fdev), (out_addr)))


#if 0 /* XXX this interface is incomplete and not used yet anyway */
/*
 * Memory bus I/O interface,
 * IID 4aa7dfa9-7c74-11cf-b500-08000953adc2.
 * This interface extends the oskit_membus interface,
 * providing additional methods allowing direct access to the bus.
 *
 * XXX This interface should probably extend oskit_bufio.
 */
struct oskit_membus_io {
	struct oskit_membus_io_ops *ops;
};
typedef struct oskit_membus_io oskit_membus_io_t;

struct oskit_membus_io_ops {

	/* Inherited IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_membus_io_t *bus,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_membus_io_t *bus);
	OSKIT_COMDECL_U	(*release)(oskit_membus_io_t *bus);

	/* Inherited oskit_device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_membus_io_t *bus,
				   oskit_devinfo_t *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_membus_io_t *fdev,
				      oskit_driver_t **out_driver);

	/* Inherited oskit_bus interface operations */
	OSKIT_COMDECL	(*getchild)(oskit_membus_io_t *bus, unsigned idx,
				     oskit_device_t **out_fdev, char *out_pos);
	OSKIT_COMDECL	(*probe)(oskit_membus_io_t *bus);

	/* Inherited oskit_membus interface operations */
	OSKIT_COMDECL	(*getchildaddr)(oskit_membus_io_t *bus, unsigned idx,
					oskit_device_t **out_fdev,
					oskit_addr_t *out_addr);

	/*** Operations specific to memory buses ***/

	/*
	 * Device drivers for devices that can exist on this bus
	 * call this method to register themselves
	 * after having found and initialized a device on this bus,
	 * or to unregister themselves if the device is removed.
	 * The 'addr' parameter must be the base physical memory address
	 * at which this device appears on this bus.
	 * If the device uses multiple physical memory address ranges,
	 * then the device's "primary" memory address should be used.
	 *
	 * Not all bus devices necessarily allow dynamic alterations;
	 * in this case this method will return OSKIT_E_DEV_FIXEDBUS.
	 * For example, the Linux SCSI adaptor drivers are tightly tied
	 * to the Linux SCSI disk/tape/etc. drivers;
	 * SCSI busses driven by these drivers will always be fixed.
	 * XXX this paragraph should be moved to dev/scsi.h.
	 */
	OSKIT_COMDECL	(*addchild)(oskit_membus_io_t *bus, oskit_addr_t addr,
				     oskit_device_t *dev);
	OSKIT_COMDECL	(*remchild)(oskit_membus_io_t *bus, oskit_addr_t addr);

	/*** Memory Address Space Management ***/

	/* Returns true if the specified address range is available. */
	oskit_bool_t
	  OSKIT_COMCALL	(*addr_avail)(oskit_membus_io_t *bus,
				      oskit_addr_t addr, oskit_size_t size);

	/*
	 * Allocate an address range in this bus's memory space.
	 * Each range has an associated owner device for bookkeeping purposes;
	 * the allocating device must pass a reference to its own device node.
	 * The addr_alloc() method returns OSKIT_E_DEV_SPACE_INUSE
	 * if any of the requested ports are already allocated.
	 */
	OSKIT_COMDECL	(*addr_alloc)(oskit_membus_io_t *bus,
				      oskit_addr_t addr, oskit_size_t size,
				      oskit_device_t *owner);

	/*
	 * Free a previously allocated port range.
	 * The 'addr' and 'size' must be _exactly_ as specified to addr_alloc():
	 * do not attempt to free part of a previously allocated range,
	 * or two consecutive ranges allocated previously, etc.
	 */
	OSKIT_COMDECL	(*addr_free)(oskit_membus_io_t *bus,
				     oskit_addr_t addr, oskit_size_t size);

	/*
	 * Scan the address space map for the first allocated range
	 * containing or above the initial value in *inout_addr,
	 * and return the actual start address, size, and owner of the range.
	 * Returns OSKIT_S_FALSE if no more allocated ranges exist.
	 */
	OSKIT_COMDECL	(*addr_scan)(oskit_membus_io_t *bus,
				     oskit_addr_t *inout_port,
				     oskit_size_t *out_size,
				     oskit_device_t **out_owner);

	/*** Memory Access and Mapping ***/

	/*
	 * Read a single byte, 16-bit, 32-bit, or 64-bit word
	 * from a particular address in this bus's physical memory space.
	 * These functions should only generally be used
	 * for occasional intermittent non-performance-critical memory access,
	 * such as during probing; for general use,
	 * it is generally much more efficent just to map the memory.
	 */
	oskit_u8_t
	  OSKIT_COMCALL	(*read8)(oskit_membus_io_t *bus, oskit_addr_t addr);
	oskit_u16_t
	  OSKIT_COMCALL	(*read16)(oskit_membus_io_t *bus, oskit_addr_t addr);
	oskit_u32_t
	  OSKIT_COMCALL	(*read32)(oskit_membus_io_t *bus, oskit_addr_t addr);
	oskit_u64_t
	  OSKIT_COMCALL	(*read64)(oskit_membus_io_t *bus, oskit_addr_t addr);

	/*
	 * Write a word of various sizes to physical memory space.
	 * The same notes apply as for the read methods above.
	 */
	OSKIT_COMDECL_V	(*write8)(oskit_membus_io_t *bus, oskit_addr_t addr,
				  oskit_u8_t val);
	OSKIT_COMDECL_V	(*write16)(oskit_membus_io_t *bus, oskit_addr_t addr,
				  oskit_u16_t val);
	OSKIT_COMDECL_V	(*write32)(oskit_membus_io_t *bus, oskit_addr_t addr,
				  oskit_u32_t val);
	OSKIT_COMDECL_V	(*write64)(oskit_membus_io_t *bus, oskit_addr_t addr,
				  oskit_u64_t val);

	/*
	 * Map a range of physical memory addresses on this bus
	 * into the local virtual virtual address space
	 * in which the client is running.
	 * The 'flags' parameter may include any of the OSKIT_MEMBUS_xxx
	 * flag bits defined below.
	 */
	OSKIT_COMDECL	(*map)(oskit_membus_io_t *bus,
				oskit_addr_t physaddr, oskit_size_t size,
				unsigned flags, void **out_virtaddr);

	/*
	 * Unmap a range previously mapped using the map() method above.
	 * The virtual address must be exactly the address returned from map(),
	 * and the size specified must be exactly as supplied to map().
	 */
	OSKIT_COMDECL	(*unmap)(oskit_membus_io_io_t *bus,
				 void *virtaddr, oskit_size_t size);
};

/* XXX extern const struct oskit_guid oskit_membus_io_iid;*/
#define OSKIT_MEMBUS_IO_IID OSKIT_GUID(0x4aa7dfa9, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

/* Flag definitions for oskit_membus->map() */
#define OSKIT_MEMBUS_WRITETHROUGH	0x01	/* No writeback caching allowed */
#define OSKIT_MEMBUS_NOCACHE	0x02	/* No caching at all allowed */

#endif /* 0 */


#if 0 /* XXX this interface isn't used, and may never be... */

/*
 * Interface to a memory-based bus device driver,
 * IID 4aa7df8a-7c74-11cf-b500-08000953adc2.
 * The oskit_membus_driver interface is exported by device drivers
 * for generic memory-mapped devices that appear in standard locations,
 * or can otherwise be probed for without additional information.
 *
 * XXX define driver priority.
 */
struct oskit_membus_driver {
	struct oskit_membus_driver_ops *ops;
};
typedef struct oskit_membus_driver oskit_membus_driver_t;

struct oskit_membus_driver_ops {

	/* Inherited IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_membus_driver_t *drv,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_membus_driver_t *drv);
	OSKIT_COMDECL_U	(*release)(oskit_membus_driver_t *drv);

	/* Inherited oskit_driver operations */
	OSKIT_COMDECL	(*getinfo)(oskit_membus_driver_t *drv,
				    oskit_devinfo_t *out_info);

	/*** Operations specific to memory-mapped device drivers ***/

	/*
	 * Probe for devices on the memory bus,
	 * creating new device nodes as appropriate
	 * and attaching them to the specified memory bus device node.
	 * On success, returns the number of devices found and attached,
	 * 0 if everything went OK but nothing was found.
	 */
	OSKIT_COMDECL	(*probe)(oskit_membus_driver_t *drv,
				 oskit_membus_t *bus);
};

/* XXX extern const struct oskit_guid oskit_membus_driver_iid; */
#define OSKIT_MEMBUS_DRIVER_IID OSKIT_GUID(0x4aa7df8a, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif /* 0 */

#endif /* _OSKIT_DEV_MEMBUS_H_ */
