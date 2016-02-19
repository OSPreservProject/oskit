/*
 * Copyright (c) 1997-1998, 2000, 2002 University of Utah and the Flux Group.
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
 * Definitions specific to Ethernet network devices.
 */
#ifndef _OSKIT_DEV_ETHERNET_H_
#define _OSKIT_DEV_ETHERNET_H_

#include <oskit/dev/device.h>
#include <oskit/io/netio.h>
#include <oskit/dev/net.h>
#include <oskit/io/bufio.h>

/* Ethernet uses 6-byte MAC addresses */
#define OSKIT_ETHERDEV_ADDR_SIZE		6

/* Some OSKIT specific flags for open */
#define OSKIT_ETHERDEV_RX_POLLING		OSKIT_NETDEV_RX_POLLING
#define OSKIT_ETHERDEV_TX_POLLING		OSKIT_NETDEV_TX_POLLING

/*
 * Standard Ethernet device node interface, derived from oskit_netdev_t,
 * IID 4aa7df98-7c74-11cf-b500-08000953adc2.
 * A device node exporting this COM interface
 * represents a single Ethernet network card/adaptor.
 */
struct oskit_etherdev {
	struct oskit_etherdev_ops *ops;
};
typedef struct oskit_etherdev oskit_etherdev_t;

struct oskit_etherdev_ops {
	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL    (*query)(oskit_etherdev_t *dev,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_etherdev_t *dev);
	OSKIT_COMDECL_U	(*release)(oskit_etherdev_t *dev);

	/* Inherited oskit_device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_etherdev_t *fdev,
				    oskit_devinfo_t *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_etherdev_t *fdev,
                                      oskit_driver_t **out_driver);

	/* Inherited oskit_netdev interface operations */
	OSKIT_COMDECL	(*open)(oskit_etherdev_t *dev, unsigned flags, 
				oskit_netio_t *recv_netio,
				oskit_netio_t **out_send_netio);

	/*
	 * Non-blocking poll for packets. The open routine will return
	 * OSKIT_E_NOTIMPL if the device does not support RX polling.
	 * A call on rxpoll to a device that does not support it, or has
	 * not been setup for polling, will return OSKIT_ENODEV. When no
	 * packets are available, returns OSKIT_ENODATA. The size of the
	 * array is passed in as *count, and when packets are available,
	 * *count is updated with the number of oskit_bufios stuffed into
	 * the array.
	 */
	OSKIT_COMDECL 	(*rxpoll)(oskit_etherdev_t *dev,
				  int *count, oskit_bufio_t *out_bufios[]);

	/*
	 * Return this Ethernet adaptor's 6-byte MAC address.
	 */
	OSKIT_COMDECL_V	(*getaddr)(oskit_etherdev_t *fdev,
				    unsigned char
				    	out_addr[OSKIT_ETHERDEV_ADDR_SIZE]);
};

/* GUID for fdev block device interface */
extern const struct oskit_guid oskit_etherdev_iid;
#define OSKIT_ETHERDEV_IID OSKIT_GUID(0x4aa7df98, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_etherdev_query(dev, iid, out_ihandle) \
	((dev)->ops->query((oskit_etherdev_t *)(dev), (iid), (out_ihandle)))
#define oskit_etherdev_addref(dev) \
	((dev)->ops->addref((oskit_etherdev_t *)(dev)))
#define oskit_etherdev_release(dev) \
	((dev)->ops->release((oskit_etherdev_t *)(dev)))
#define oskit_etherdev_getinfo(fdev, out_info) \
	((fdev)->ops->getinfo((oskit_etherdev_t *)(fdev), (out_info)))
#define oskit_etherdev_getdriver(fdev, out_driver) \
	((fdev)->ops->getdriver((oskit_etherdev_t *)(fdev), (out_driver)))
#define oskit_etherdev_open(dev, flags, recv_netio, out_send_netio) \
	((dev)->ops->open((oskit_etherdev_t *)(dev), (flags), (recv_netio), (out_send_netio)))
#define oskit_etherdev_getaddr(fdev, out_addr) \
	((fdev)->ops->getaddr((oskit_etherdev_t *)(fdev), (out_addr)))
#define oskit_etherdev_rxpoll(dev, count, bufios) \
	((dev)->ops->rxpoll((oskit_etherdev_t *)(dev), (count), (bufios)))

#endif /* _OSKIT_DEV_ETHERNET_H_ */
