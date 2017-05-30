/*
 * Copyright (c) 1997-2000 The University of Utah and the Flux Group.
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

#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>

#include "net.h"
#include "ethernet.h"
#include "osenv.h"

struct oskit_guid oskit_linux_ether_iids[ETHER_NIIDS] = {
	OSKIT_DEVICE_IID,
	OSKIT_NETDEV_IID,
	OSKIT_ETHERDEV_IID
};


/*** Ethernet adaptor device node interface ***/

static OSKIT_COMDECL
ether_query(oskit_etherdev_t *intf, const struct oskit_guid *iid,
	    void **out_ihandle)
{
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_device_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_netdev_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_etherdev_iid, sizeof(*iid)) == 0) {
		*out_ihandle = intf;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_V
ether_getaddr(oskit_etherdev_t *intf,
	       unsigned char out_addr[OSKIT_ETHERDEV_ADDR_SIZE])
{
	struct net_alias *dev = (struct net_alias*)((char*)intf - DEVI_OFS);

	memcpy(out_addr, dev->ldev->dev_addr, OSKIT_ETHERDEV_ADDR_SIZE);
}

struct oskit_etherdev_ops oskit_linux_ether_ops = {
	ether_query,
	(void*)oskit_linux_netdev_addref, (void*)oskit_linux_netdev_release,
	(void*)oskit_linux_netdev_getinfo, (void*)oskit_linux_netdev_getdriver,
	(void*)oskit_linux_netdev_open,
	(void*)oskit_linux_netdev_rxpoll,
	ether_getaddr
};

