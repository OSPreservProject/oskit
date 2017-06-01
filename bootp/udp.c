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

/* Modified from FreeBSD 2.1 sys/i386/boot/netboot */
/**************************************************************************
NETBOOT -  BOOTP/TFTP Bootstrap Program

Author: Martin Renters
  Date: Dec/93

**************************************************************************/

#include <oskit/c/arpa/inet.h>
#include <oskit/c/string.h>
#include <oskit/c/assert.h>
#include <stdio.h>
#include "udp.h"
#include "bootp.h"

/*
 * IP checksum algorithm.
 */
unsigned short
bootp_ipchksum(void *ipv, int len)
{
	unsigned short *ip = ipv;
	unsigned long sum = 0;

	len >>= 1;
	while (len--) {
		sum += *(ip++);
		if (sum > 0xFFFF)
			sum -= 0xFFFF;
	}
	return((~sum) & 0x0000FFFF);
}



/**************************************************************************
UDP_TRANSMIT - Send a UDP datagram
**************************************************************************/
int
bootp_udp_broadcast(char *ether_src, 
	unsigned long srcip,
	unsigned short srcsock, 
	unsigned short destsock, int len, void *bufv)
{
	static char ether_bcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	char *buf = bufv;
	struct iphdr *ip;
	struct udphdr *udp;
	oskit_u32_t temp;

	ip = (struct iphdr *)buf;
	udp = (struct udphdr *)(buf + sizeof(struct iphdr));
	ip->verhdrlen = 0x45;
	ip->service = 0;
	ip->len = htons(len);
	ip->ident = 0;
	ip->frags = 0;
	ip->ttl = 60;
	ip->protocol = IP_UDP;
	ip->chksum = 0;

	temp = srcip;
	memcpy(ip->src, &temp, 4);
	temp = IP_BROADCAST;
	memcpy(ip->dest, &temp, 4);

	ip->chksum = bootp_ipchksum(buf, sizeof(struct iphdr));
	udp->src = htons(srcsock);
	udp->dest = htons(destsock);
	udp->len = htons(len - sizeof(struct iphdr));
	udp->chksum = 0;
	bootp_eth_transmit(ether_src, ether_bcast, ETHERTYPE_IP, len, buf);
	return(1);
}

