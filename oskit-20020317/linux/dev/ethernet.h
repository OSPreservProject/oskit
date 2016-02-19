/*
 * Copyright (c) 1997, 1998 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
#ifndef _LINUX_DEV_ETHERNET_H_
#define _LINUX_DEV_ETHERNET_H_

#include <linux/netdevice.h>
#include <linux/version.h>

#include <oskit/dev/net.h>
#include <oskit/io/netio.h>

#include "glue.h"
#include "net.h"

extern struct oskit_etherdev_ops oskit_linux_ether_ops;

#define ETHER_NIIDS 3
extern struct oskit_guid oskit_linux_ether_iids[ETHER_NIIDS];

#define driver(name, description, vendor, author, filename, probe)	\
	extern int probe(struct device *dev);				\
	static struct net_driver fdev_drv = {				\
		{ { &oskit_linux_driver_ops },				\
		  oskit_linux_netdev_probe,					\
		  { #name, description " Ethernet driver", vendor,	\
		    author, "Linux "UTS_RELEASE } },			\
		{ #name, description " Ethernet adaptor", vendor,	\
		  author, "Linux "UTS_RELEASE },			\
		(struct oskit_netdev_ops*)&oskit_linux_ether_ops,		\
		"eth",							\
		oskit_linux_ether_iids, ETHER_NIIDS,			\
		probe };						\
	oskit_error_t oskit_linux_init_ethernet_##name(void) {		\
		return oskit_linux_driver_register(&fdev_drv.ds);	\
	}

#endif /* _LINUX_DEV_ETHERNET_H_ */
