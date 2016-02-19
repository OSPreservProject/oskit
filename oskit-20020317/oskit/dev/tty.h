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
 * Definition of the oskit_ttydev interface
 * representing traditional POSIX/Unix TTY-like devices,
 * such as character-based consoles or serial ports.
 *
 * Note that this interface is decidedly non-ideal
 * as a general interface for all character-oriented devices,
 * because it is too high-level and brings a lot of traditional Unix baggage.
 * For example, it assumes that the driver handles things such as
 * cooked-mode I/O (e.g., line editing), timeouts, etc.;
 * furthermore it follows the grand Unix tradition
 * of trying to make everything look like a serial port.
 * This interface is appropriate to be provided by existing drivers
 * such as those we steal from BSD or Linux,
 * because those drivers already have all the tty glop built in;
 * however, we will also probably want to define
 * several smaller, simpler interfaces that directly abstract the hardware
 * without forcing all drivers to implement high-level Unixisms.
 */
#ifndef _OSKIT_DEV_TTY_H_
#define _OSKIT_DEV_TTY_H_

#include <oskit/dev/device.h>

struct oskit_ttystream;


/*
 * Standard TTY device node interface, derived from oskit_device_t,
 * IID 4aa7dfa6-7c74-11cf-b500-08000953adc2.
 * Various tidbits of information can be obtained through this interface;
 * however, in order to read and write the device itself,
 * the open() method in this interface must be called
 * to obtain a per-open ttystream object (see io/ttystream.h).
 * Opening the device indicates that it is about to be used;
 * this typically causes the driver to allocate additional resources
 * necessary to access the device rather than merely "know about" it.
 * The device driver isn't obligated to do anything in particular, however;
 * in fact, it may simply export the ttystream interface
 * as part of the device node object itself,
 * and return a reference to that as the result of the open() operation.
 */
struct oskit_ttydev
{
	struct oskit_ttydev_ops *ops;
};
typedef struct oskit_ttydev oskit_ttydev_t;

struct oskit_ttydev_ops
{
	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_ttydev_t *dev, const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_ttydev_t *dev);
	OSKIT_COMDECL_U	(*release)(oskit_ttydev_t *dev);

	/* Base fdev device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_ttydev_t *fdev, oskit_devinfo_t *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_ttydev_t *fdev,
				      oskit_driver_t **out_driver);

	/* TTY device interface operations */

	/*
	 * Open this device in the standard active "callout" mode,
	 * The open will generally complete immediately
	 * unless the device is already in exclusive use,
	 * in which it fails with OSKIT_EBUSY.
	 */
	OSKIT_COMDECL	(*open)(oskit_ttydev_t *dev, oskit_u32_t flags,
				struct oskit_ttystream **out_ttystream);

	/*
	 * Open this device in passive "callin" mode.
	 * The open will block until a call is received
	 * (e.g., for serial ports, until Carrier Detect is asserted).
	 * Devices that don't support callins simply return OSKIT_E_NOTIMPL.
	 */
	OSKIT_COMDECL	(*listen)(oskit_ttydev_t *dev, oskit_u32_t flags,
				  struct oskit_ttystream **out_ttystream);
};

/* GUID for fdev block device interface */
extern const struct oskit_guid oskit_ttydev_iid;
#define OSKIT_TTYDEV_IID OSKIT_GUID(0x4aa7dfa6, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_ttydev_query(dev, iid, out_ihandle) \
	((dev)->ops->query((oskit_ttydev_t *)(dev), (iid), (out_ihandle)))
#define oskit_ttydev_addref(dev) \
	((dev)->ops->addref((oskit_ttydev_t *)(dev)))
#define oskit_ttydev_release(dev) \
	((dev)->ops->release((oskit_ttydev_t *)(dev)))
#define oskit_ttydev_getinfo(fdev, out_info) \
	((fdev)->ops->getinfo((oskit_ttydev_t *)(fdev), (out_info)))
#define oskit_ttydev_getdriver(fdev, out_driver) \
	((fdev)->ops->getdriver((oskit_ttydev_t *)(fdev), (out_driver)))
#define oskit_ttydev_open(dev, flags, out_ttystream) \
	((dev)->ops->open((oskit_ttydev_t *)(dev), (flags), (out_ttystream)))
#define oskit_ttydev_listen(dev, flags, out_ttystream) \
	((dev)->ops->listen((oskit_ttydev_t *)(dev), (flags), (out_ttystream)))

#endif /* _OSKIT_DEV_TTY_H_ */
