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
 * bootp.c 
 *
 * This shows how to use the bootp library
 */
#include <oskit/net/bootp.h>

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>

#include "bootp.h"

/*
 * read an IP address from the console
 */
void
get_net_addr(char *buf)
{
	struct in_addr ip;

	do {
		gets(buf);
	} while (inet_aton(buf, &ip) == 0);
}

/*
 * first try to contact bootp server for ip addr, if 
 * that fails, ask the user
 */
void
get_ipinfo(oskit_etherdev_t *dev, char *ipaddr, char *gateway, char *netmask,
	long *howfarwest)
{
	struct bootp_net_info bootpinfo;
	int err;

	/* get the IP address & other info */
	err = bootp(dev, &bootpinfo);

	if (!err) {
		bootp_dump(&bootpinfo);

		if (ipaddr && (bootpinfo.flags & BOOTP_NET_IP))
			strcpy(ipaddr, inet_ntoa(bootpinfo.ip));

		if (gateway && (bootpinfo.flags & BOOTP_NET_GATEWAY))
			strcpy(gateway, inet_ntoa(bootpinfo.gateway.addr[0]));

		if (netmask) {
			if (bootpinfo.flags & BOOTP_NET_NETMASK)
				strcpy(netmask, inet_ntoa(bootpinfo.netmask));
			else
				strcpy(netmask, inet_ntoa(
					bootp_default_netmask(bootpinfo.ip)));
                }
		if (howfarwest)
			*howfarwest = (bootpinfo.flags & BOOTP_NET_TIME_OFFSET)
				? bootpinfo.time_offset : 0;

		bootp_free(&bootpinfo);

	} else {
		if (ipaddr) {
			printf("What is my IP address: ");
			get_net_addr(ipaddr);
		}

		if (gateway) {
			printf("What is my default gateway: ");
			get_net_addr(gateway);
		}

		if (netmask) {
			printf("What is my netmask: ");
			get_net_addr(netmask);
		}
	}
}
