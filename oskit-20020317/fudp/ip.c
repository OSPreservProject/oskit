/*
 * Copyright (c) 1998 The University of Utah and the Flux Group.
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
 * The IP layer.
 */
#include <oskit/io/netio.h>
#include <oskit/io/bufio.h>
#include <oskit/net/ether.h>
#include <oskit/c/netinet/in.h>
#include <oskit/c/netinet/ip.h>
#include <oskit/c/arpa/inet.h>
#include <oskit/c/string.h>

#include <oskit/fudp.h>
#include "fudp.h"

static unsigned short ipid = 23;

/*
 * IP checksum algorithm.
 */
static unsigned short
ipcksum(void *ipv, int len)
{
	unsigned short *ip = ipv;
	unsigned long sum = 0;

	len >>= 1;
	while (len--) {
		sum += *(ip++);
		if (sum > 0xffff)
			sum -= 0xffff;
	}
	return((~sum) & 0x0000ffff);
}

/*
 * Send a fudp buf via IP.
 * Fill in the IP header and pass to eth_transmit.
 * Deal with broadcasting and gateways.
 * Note: `buflen' is the length of the fudp buffer, not just the UDP part.
 *	 `from' and `to' are expected to be in network order.
 */
oskit_error_t
ip_send(oskit_netio_t *netio,
	const struct in_addr *from,
	const struct in_addr *to,
	oskit_bufio_t *b,
	oskit_size_t blen)
{
	oskit_error_t err = 0;
	unsigned char eth_broadcast[] = ETHER_BCAST;
	unsigned char eth_src[ETHER_ADDR_SIZE];
	unsigned char eth_dst[ETHER_ADDR_SIZE];
	struct ip *ip;

	/*
	 * Map in the bufio so `ip' points to the IP part.
	 */
	err = oskit_bufio_map(b, (void **)&ip,
			      sizeof(struct ether_header),
			      blen - sizeof(struct ether_header));
	if (err)
		return err;

	/*
	 * Fill in the IP header.
	 */
	ip->ip_v	  = IPVERSION;
	ip->ip_hl	  = (sizeof *ip) >> 2;
	ip->ip_tos	  = 0;
	ip->ip_len	  = htons(blen - sizeof(struct ether_header));
	ip->ip_id	  = ipid++;	/* apparently can't leave this
					   zero or we get dropped by FreeBSD  */
	ip->ip_off	  = 0;
	ip->ip_ttl	  = 64;
	ip->ip_p	  = IPPROTO_UDP;
	ip->ip_sum	  = 0;
	ip->ip_src.s_addr = from->s_addr;	/* already in net order */
	ip->ip_dst.s_addr = to->s_addr;		/* already in net order */
	ip->ip_sum	  = ipcksum(ip, sizeof *ip);

	if (farp_lookup(&ip->ip_dst, eth_dst) != 0 ||
	    farp_lookup(&ip->ip_src, eth_src) != 0) {
		err = OSKIT_EIO;	/* XXX whatever */
		goto done;
	}

	if (ip->ip_dst.s_addr == htons(INADDR_BROADCAST)) {
		memcpy(eth_dst, eth_broadcast, ETHER_ADDR_SIZE);
		goto sendit;
	}

#if 0
	/* Deal with gateway - maybe should do this a layer up? */
	if ((ntohl(ip->src) & ?MYNETMASK?) != (ntohl(ip->dst) & ?MYNETMASK?))
		send to the gateway;
#endif

 sendit:
	err = eth_transmit(netio, eth_src, eth_dst, ETHERTYPE_IP, b, blen);
 done:
	oskit_bufio_unmap(b, ip,
			  sizeof(struct ether_header),
			  blen - sizeof(struct ether_header));

	return err;
}
