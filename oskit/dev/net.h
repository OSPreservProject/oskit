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
 * Definitions specific to network devices.
 */
#ifndef _OSKIT_DEV_NET_H_
#define _OSKIT_DEV_NET_H_

#include <oskit/io/netio.h>
#include <oskit/dev/device.h>
#include <oskit/io/bufio.h>

/*
 * Some OSKIT specific flags for open
 *
 * RX_POLLING puts the device in polled receive mode, if it supports it.
 * Packets can then polled using the rxpoll method. No interrupts will be
 * generated for incoming packets, so using this feature requires that
 * packets be polled on a timely basis to avoid lots of network lossage.
 *
 * TX_POLLING disables transmitter-done interrupts, if it supports it.
 * TxDone packets are instead reaped as necessary when netio_push is called.
 */
#define OSKIT_NETDEV_RX_POLLING		0x01
#define OSKIT_NETDEV_TX_POLLING		0x02

/*
 * Standard network device node interface, derived from oskit_device_t,
 * IID 4aa7df8c-7c74-11cf-b500-08000953adc2.
 */
struct oskit_netdev {
	struct oskit_netdev_ops *ops;
};
typedef struct oskit_netdev oskit_netdev_t;

struct oskit_netdev_ops {
	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL    (*query)(oskit_netdev_t *dev, const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_netdev_t *dev);
	OSKIT_COMDECL_U	(*release)(oskit_netdev_t *dev);

	/* Base fdev device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_netdev_t *fdev, oskit_devinfo_t *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_netdev_t *fdev,
                                      oskit_driver_t **out_driver);

	/*
	 * Open this network interface and prepare to receive packets.
	 * A reference to a netio interface must be passed as a parameter;
	 * the driver will call recv_netio->push() when a packet arrives.
	 * This function also returns a reference to a netio interface
	 * implemented by the _driver_ (send_netio);
	 * the caller invokes send_netio->push() to send a packet.
	 */
	OSKIT_COMDECL	(*open)(oskit_netdev_t *dev, unsigned flags, 
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
	OSKIT_COMDECL 	(*rxpoll)(oskit_netdev_t *dev,
				  int *count, oskit_bufio_t *out_bufios[]);
};

/* GUID for fdev block device interface */
extern const struct oskit_guid oskit_netdev_iid;
#define OSKIT_NETDEV_IID OSKIT_GUID(0x4aa7df8c, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_netdev_query(dev, iid, out_ihandle) \
	((dev)->ops->query((oskit_netdev_t *)(dev), (iid), (out_ihandle)))
#define oskit_netdev_addref(dev) \
	((dev)->ops->addref((oskit_netdev_t *)(dev)))
#define oskit_netdev_release(dev) \
	((dev)->ops->release((oskit_netdev_t *)(dev)))
#define oskit_netdev_getinfo(fdev, out_info) \
	((fdev)->ops->getinfo((oskit_netdev_t *)(fdev), (out_info)))
#define oskit_netdev_getdriver(fdev, out_driver) \
	((fdev)->ops->getdriver((oskit_netdev_t *)(fdev), (out_driver)))
#define oskit_netdev_open(dev, flags, recv_netio, out_send_netio) \
	((dev)->ops->open((oskit_netdev_t *)(dev), (flags), (recv_netio), (out_send_netio)))
#define oskit_netdev_rxpoll(dev, count, bufios) \
	((dev)->ops->rxpoll((oskit_netdev_t *)(dev), (count), (bufios)))

#endif /* _OSKIT_DEV_NET_H_ */
