/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * Code derived from boot/net/driver.c
 */

#include <oskit/c/stdio.h>
#include <oskit/net/bootp.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>
#include <oskit/c/arpa/inet.h>

#include <oskit/debug.h>

// ToDo: locate these properly
#include <boot/net/ether.h>

extern oskit_etherdev_t *ether_dev;
extern struct bootp_net_info bootp_info;

char             *hostname    = 0;        /* padded with 0's to hostnamelen  */
int               hostnamelen = 0;        /* word aligned length of hostname */
unsigned long     netmask     = 0;        /* in host order */
struct arptable_t arptable[MAX_ARP] = {}; /* in host order */

/*
 * Flags required by the initialiser.
 * This should be imported by bootp_init.
 */
const unsigned int required_flags = (
        BOOTP_NET_IP |
        BOOTP_NET_NETMASK |
        BOOTP_NET_GATEWAY |
        BOOTP_NET_HOSTNAME );

oskit_error_t
init(void)
{
        assert( (~bootp_info.flags & required_flags) == 0 );
        assert( ether_dev );

	bootp_info.ip.s_addr = ntohl(bootp_info.ip.s_addr);
	memcpy(&arptable[ARP_CLIENT].ipaddr, 
               &bootp_info.ip.s_addr,
	       sizeof bootp_info.ip.s_addr);

	bootp_info.netmask.s_addr = ntohl(bootp_info.netmask.s_addr);
	memcpy(&netmask, 
               &bootp_info.netmask.s_addr, 
               sizeof bootp_info.netmask.s_addr);

	bootp_info.gateway.addr[0].s_addr = ntohl(bootp_info.gateway.addr[0].s_addr);
	memcpy(&arptable[ARP_GATEWAY].ipaddr, 
               &bootp_info.gateway.addr[0].s_addr,
	       sizeof bootp_info.gateway.addr[0].s_addr);

	/* hostnamelen needs to be word aligned. */
	hostname = (char *)calloc(strlen(bootp_info.hostname) + 3, 1);
        if (!hostname) return OSKIT_ENOMEM;
	strcpy(hostname, bootp_info.hostname);
	hostnamelen = (strlen(hostname) + 3) & ~3;

	oskit_etherdev_getaddr(ether_dev, arptable[ARP_CLIENT].node);
        { 
                int i;
                printf("Ethaddr: ");
                for(i=0; i<6; ++i) {
                        printf("%d.",arptable[ARP_CLIENT].node[i]);
                }
                printf("\n");
        }

	return 0;
}

oskit_error_t
fini(void)
{
        if (hostname)
                free(hostname);
        return 0;
}
