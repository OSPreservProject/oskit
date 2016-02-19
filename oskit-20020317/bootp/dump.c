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

#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/netinet/in.h>
#include <oskit/c/arpa/inet.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/error.h>

#include "bootp.h"

static
char *print_array(struct bootp_addr_array arr)
{
	/* another 256 bytes down the drain... */
	static char buf[256];

	char   *p = buf;
	struct in_addr	*a = arr.addr;
	int    len = arr.len;
	char   sep = ' ';

	while (len-- && p < buf + sizeof buf - 12)
		p += sprintf(p, "%c%s", sep, inet_ntoa(*a++)), sep = ',';
	return buf;
}

void
bootp_dump(struct bootp_net_info *info)
{
	printf(PRF"Dumping contents of bootp_net_info @%p\n", info);

	printf(PRF"Server was "); 
	if (info->flags & BOOTP_NET_SERVER_NAME)
		printf("%s ", info->server_name);
	if (info->flags & BOOTP_NET_SERVER)
		printf(" %s ", inet_ntoa(info->server));
	if (info->flags & BOOTP_NET_SERVER_ADDR)
		printf(" %02x:%02x:%02x:%02x:%02x:%02x",
			info->server_hwaddr[0],
			info->server_hwaddr[1],
			info->server_hwaddr[2],
			info->server_hwaddr[3],
			info->server_hwaddr[4],
			info->server_hwaddr[5]);
	printf("\n");

	if (info->flags & BOOTP_NET_HOSTNAME)
		printf(PRF"Hostname: %s\n", info->hostname);

	printf(PRF);
	if (info->flags & BOOTP_NET_IP)
		printf("IP: %s ",  inet_ntoa(info->ip));
	if (info->flags & BOOTP_NET_NETMASK)
		printf("Netmask: %s ",  inet_ntoa(info->netmask));
	if (info->flags & BOOTP_NET_GATEWAY)
		printf("Gateway(s): %s ",  print_array(info->gateway));
	printf("\n");

	if (info->flags & BOOTP_NET_DOMAINNAME)
		printf(PRF"Domainname: %s\n",  info->domainname);

	if (info->flags & BOOTP_NET_DNS_SERVER)
		printf(PRF"DNS server(s): %s\n", print_array(info->dns_server));

	if (info->flags & BOOTP_NET_TIME_SERVER)
		printf(PRF"Time server(s): %s\n", 
			print_array(info->time_server));

	if (info->flags & BOOTP_NET_LOG_SERVER)
		printf(PRF"Log server(s): %s\n", print_array(info->log_server));

	if (info->flags & BOOTP_NET_LPR_SERVER)
		printf(PRF"LPR server(s): %s\n", print_array(info->lpr_server));

	if (info->flags & BOOTP_NET_TIME_OFFSET)
		printf(PRF"Time offset from UTC is %d seconds west\n", 
			info->time_offset);

	if (info->flags & BOOTP_NET_BOOTFILE_NAME)
		printf(PRF"Bootfile name is %s\n", info->bootfile_name);

	printf(PRF"End of dump\n");
}

