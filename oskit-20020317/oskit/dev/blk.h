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
 * Definitions specific to block devices.
 */
#ifndef _OSKIT_DEV_BLK_H_
#define _OSKIT_DEV_BLK_H_

#include <oskit/dev/device.h>

struct oskit_blkio;


/*
 * Standard block device node interface, derived from oskit_device_t,
 * IID 4aa7df82-7c74-11cf-b500-08000953adc2.
 * Various tidbits of information can be obtained through this interface;
 * however, in order to read and write the device itself,
 * the open() method in this interface must be called
 * to obtain a per-open blkio object
 * conforming to the absolute block I/O interface defined in blkio.h.
 * Opening the device indicates that it is about to be used;
 * this typically causes the driver to allocate additional resources
 * necessary to access the device rather than merely "know about" it.
 * The device driver isn't obligated to do anything in particular, however;
 * in fact, it may simply export the blkio interface
 * as part of the device node object itself,
 * and return a reference to that as the result of the open() operation.
 */
struct oskit_blkdev
{
	struct oskit_blkdev_ops *ops;
};
typedef struct oskit_blkdev oskit_blkdev_t;

struct oskit_blkdev_ops
{
	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_blkdev_t *dev, const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_blkdev_t *dev);
	OSKIT_COMDECL_U	(*release)(oskit_blkdev_t *dev);

	/* Base fdev device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_blkdev_t *fdev, oskit_devinfo_t *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_blkdev_t *fdev,
				      oskit_driver_t **out_driver);

	/* Block device interface operations */
	OSKIT_COMDECL	(*open)(oskit_blkdev_t *dev, unsigned mode,
				struct oskit_blkio **out_blkio);
};

/* GUID for fdev block device interface */
extern const struct oskit_guid oskit_blkdev_iid;
#define OSKIT_BLKDEV_IID OSKIT_GUID(0x4aa7df82, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_blkdev_query(dev, iid, out_ihandle) \
	((dev)->ops->query((oskit_blkdev_t *)(dev), (iid), (out_ihandle)))
#define oskit_blkdev_addref(dev) \
	((dev)->ops->addref((oskit_blkdev_t *)(dev)))
#define oskit_blkdev_release(dev) \
	((dev)->ops->release((oskit_blkdev_t *)(dev)))
#define oskit_blkdev_getinfo(fdev, out_info) \
	((fdev)->ops->getinfo((oskit_blkdev_t *)(fdev), (out_info)))
#define oskit_blkdev_getdriver(fdev, out_driver) \
	((fdev)->ops->getdriver((oskit_blkdev_t *)(fdev), (out_driver)))
#define oskit_blkdev_open(dev, mode, out_blkio) \
	((dev)->ops->open((oskit_blkdev_t *)(dev), (mode), (out_blkio)))

#endif /* _OSKIT_DEV_BLK_H_ */
