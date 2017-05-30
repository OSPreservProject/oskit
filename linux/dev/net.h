/*
 * Copyright (c) 1997, 1998, 2000 The University of Utah and the Flux Group.
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
/*
 * Linux net emulation definitions.
 */
#ifndef _LINUX_NET_H_
#define _LINUX_NET_H_

#include <linux/netdevice.h>

#include <oskit/io/bufio.h>
#include <oskit/io/netio.h>

#include "glue.h"


/*
 * The automatically generated glue file for each network driver
 * statically declares an instance of this structure,
 * using the driver() macro
 * defined in the appropriate network type-specific header
 * (e.g., ethernet.h).
 */
struct net_driver {
	struct driver_struct	ds;
	oskit_devinfo_t		dev_info;
	struct oskit_netdev_ops	*dev_ops;
	const char		*basename;
	oskit_guid_t		*dev_iids;
	int			dev_niids;
	int			(*probe)(struct device *dev);
};


/*
 * Each network device we find is represented by one of these structures,
 * which is attached one-to-one to each Linux 'struct device'.
 *
 * Since we don't want to modify Linux's header files,
 * and don't want Linux header files to have to cross-include ours,
 * we can't simply add the contents of this struct to the device struct.
 * It turns out we can't just wrap a device struct in our own struct either,
 * because although it appears that under the "standard" practice
 * we (the glue code) get to allocate the device struct and could extend it,
 * some Linux net drivers play fast and loose and allocate their own.
 * So instead we just maintain our own separate struct.
 *
 * Conveniently, Linux's device struct contains a pointer called 'my_alias'
 * to a structure called 'net_alias', which in Linux is used
 * only by the higher-level network load balancing code;
 * we've commandeered this struct name for our own purposes
 * and use the 'my_alias' pointer in the device struct
 * as a back-pointer to our own device structure.
 * Wonder of wonders, the name is even semi-appropriate.
 */
struct net_alias {
	struct device	*ldev;		/* Linux net device struct */
	struct net_driver *drv;		/* Driver struct for this device */
	oskit_netdev_t	devi;		/* Exported device node interface */

	/*
	 * The following fields represent the per-open device state;
	 * they are simply part of the ether_device structure
	 * because we can fundamentally have only one opener at a time.
	 * (Packet demultiplexing must be done at a higher level.)
	 * A network device is "open" when send_ioi_count > 0.
	 */
	oskit_netio_t	send_ioi;	/* Exported net I/O intf for send */
	unsigned 	send_ioi_count;	/* Reference count for send_ioi */
	oskit_netio_t	*recv_ioi;	/* Imported net I/O intf for recv */
	unsigned	openflags;	/* Stuff */
};

/*
 * These offsets are used to find the ether_device
 * given a pointer to the exported etheri or send_ioi interfaces.
 */
#define DEVI_OFS	((int)&((struct net_alias*)0)->devi)
#define SEND_IOI_OFS	((int)&((struct net_alias*)0)->send_ioi)


oskit_error_t oskit_linux_net_init(void);

void *oskit_linux_skbmem_alloc(unsigned int hdrsize, unsigned int size,
			 int flags, unsigned int *out_sizep);
void oskit_linux_skbmem_free(void *ptr, unsigned int size);

/* Common fdev probe routine for all Linux network drivers */
oskit_error_t oskit_linux_netdev_probe(struct driver_struct *ds);

/* Default implementations for oskit_netdev_t interface methods */
OSKIT_COMDECL_U oskit_linux_netdev_addref(oskit_netdev_t *intf);
OSKIT_COMDECL_U oskit_linux_netdev_release(oskit_netdev_t *intf);
OSKIT_COMDECL oskit_linux_netdev_getinfo(oskit_netdev_t *intf,
					oskit_devinfo_t *out_info);
OSKIT_COMDECL oskit_linux_netdev_getdriver(oskit_netdev_t *intf,
					  oskit_driver_t **out_driver);
OSKIT_COMDECL oskit_linux_netdev_open(oskit_netdev_t *intf, unsigned flags,
				    oskit_netio_t *recv_net_io,
				    oskit_netio_t **out_send_net_io);
OSKIT_COMDECL oskit_linux_netdev_rxpoll(oskit_netdev_t *intf, 
				    int *count, oskit_bufio_t *out_bufios[]);

/*
 * SKB helper.
 */
struct sk_buff	*skbuff_is_skbufio(oskit_bufio_t *io);

#endif /* _LINUX_NET_H_ */
