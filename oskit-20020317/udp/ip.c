/*
 * Copyright (c) 1998, 2001 The University of Utah and the Flux Group.
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
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <oskit/udplib.h>
#include "udplib.h"

static unsigned short ipid = 23;
static void icmp_interrupt(oskit_bufio_t *b, oskit_size_t pkt_size);

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
ip_send(const struct in_addr *from,
	const struct in_addr *to,
	const struct in_addr *mask,
	const struct in_addr *gateway,
	oskit_bufio_t *b,
	oskit_size_t blen)
{
	oskit_error_t err = 0;
	unsigned char eth_broadcast[] = ETHER_BCAST;
	unsigned char eth_src[ETHER_ADDR_SIZE];
	unsigned char eth_dst[ETHER_ADDR_SIZE];
	struct ip *ip;
	struct in_addr destip;

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

	/*
	 * Copy addresses with memcpy for the ARM, since they are not
	 * longword aligned (since ip is not).
	 *
	 * Both addresses are already in network order.
	 */
	memcpy(&ip->ip_src, from, sizeof ip->ip_src);
	memcpy(&ip->ip_dst, to, sizeof ip->ip_dst);

	ip->ip_sum	  = ipcksum(ip, sizeof *ip);

	if (farp_lookup(from, eth_src) != 0)
		panic("We should have our own (%s) ARP entry!",
		      inet_ntoa(*from));

	/*
	 * Figure out the destination.  It will be one of:
	 *	- the gateway eth addr, if specified dest not within subnet
	 *	- the broadcast eth addr, if specified dest is subnet broadcast
	 *	- the specified dest eth addr otherwise
	 */
	if ((to->s_addr & mask->s_addr) != (from->s_addr & mask->s_addr)) {
		if ((gateway->s_addr & mask->s_addr) !=
		    (from->s_addr & mask->s_addr))
			panic("Gateway not on subnet!");
		destip.s_addr = gateway->s_addr;
	} else
		destip.s_addr = to->s_addr;

	if ((destip.s_addr & ~mask->s_addr) == (INADDR_BROADCAST & ~mask->s_addr))
		memcpy(eth_dst, eth_broadcast, ETHER_ADDR_SIZE);
	else {
		int		retries = 5;

		while (retries) {
			if (farp_lookup(&destip, eth_dst) == 0)
				goto sendit;
			arpresolve(&destip);
			retries--;
		}
		printf("ARP for %s timed out\n", inet_ntoa(destip));
		err = OSKIT_EHOSTUNREACH;
		goto done;
	}
 sendit:
	err = eth_transmit(eth_src, eth_dst, ETHERTYPE_IP, b, blen);
 done:
	oskit_bufio_unmap(b, ip,
			  sizeof(struct ether_header),
			  blen - sizeof(struct ether_header));

	return err;
}

/*
 * Upcall from eth layer.
 */
void
ip_interrupt(oskit_bufio_t *b, oskit_size_t pkt_size)
{
	struct ether_header	*eth;
	struct ip		*ip;
	oskit_error_t		err;
	int			ip_proto;

	err = oskit_bufio_map(b, (void **)&eth, 0, pkt_size);
	assert(err == 0);

	ip  = (struct ip *)(eth + 1);
	ip_proto = ip->ip_p;
	
	err = oskit_bufio_unmap(b, (void *)eth, 0, pkt_size);
	assert(err == 0);

	switch (ip_proto) {

	case IPPROTO_UDP:
		socket_interrupt(b, pkt_size);
		break;

	case IPPROTO_ICMP:
		icmp_interrupt(b, pkt_size);
		break;
	}
}

/*
 * ICMP support. Basically, ping and ipod.
 */

/*
 * There is no suitable header file in oskit/net. I stick one in at some
 * point, when I clean up this code some more.
 */
struct icmp {
        unsigned char	icmp_type;	/* type of message, see below */
#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0
#define ICMP_IPOD 6
        unsigned char	icmp_code;	/* type sub code */
#define ICMP_IPOD_IPOD 6
        unsigned short	icmp_cksum;	/* ones complement cksum of struct */
	unsigned short	icmp_id;
	unsigned short	icmp_seq;
};

static void
icmp_interrupt(oskit_bufio_t *b, oskit_size_t pkt_size)
{
	struct ether_header	*eth;
	char			eth_dst[ETHER_ADDR_SIZE];
	char			buf[1024];
	struct ip		*ip;
	struct icmp		*icmp;
	oskit_error_t		err;
	int			hlen, dlen;

	err = oskit_bufio_map(b, (void **)&eth, 0, pkt_size);
	assert(err == 0);

	ip   = (struct ip *)(eth + 1);
	hlen = ip->ip_hl << 2;

	/*
	 * Must copy out the ip header because of alignment. Once we do
	 * that we can get the len field, and then copy out all of it.
	 */
	memcpy(buf, (eth + 1), hlen);
	ip   = (struct ip *) buf;
	if (ipcksum(ip, hlen) != 0)
		return;

	memcpy(buf, (eth + 1), ntohs(ip->ip_len));
	icmp = (struct icmp *) ((char *)ip + hlen);
	dlen = ntohs(ip->ip_len) - hlen;
	if (ipcksum(icmp, dlen) != 0)
		return;

	if (icmp->icmp_type == ICMP_ECHO) {
		struct in_addr myip;
		
		icmp->icmp_type  = ICMP_ECHOREPLY;
		icmp->icmp_cksum = 0;
		icmp->icmp_cksum = ipcksum(icmp, dlen);

		ip->ip_id  = htons(ipid++);
		ip->ip_dst = ip->ip_src;

		/*
		 * The dst could be a broadcast address, so must use
		 * our addresses explicitly.
		 */
		myipaddr(&myip);
		ip->ip_src.s_addr = htonl(myip.s_addr);

		ip->ip_sum = 0;
		ip->ip_sum = ipcksum(ip, hlen);

		/*
		 * Copy packet back into original bufio (reuse it).
		 * It will be released after it is sent since we have not
		 * added any references.
		 */
		memcpy((void *)(eth + 1), ip, ntohs(ip->ip_len));

		/*
		 * We go through the eth transmit code directly since
		 * there is no gateway/routing code down there.
		 * A zero src ether address tells the eth code to fill it in.
		 * Also kludgy.
		 */
		memcpy(eth_dst, eth->ether_shost, ETHER_ADDR_SIZE);
		eth_transmit(0, eth_dst, ETHERTYPE_IP, b, pkt_size);
		return;
	}
#ifdef  UTAHTESTBED
#define PODHOST "155.101.128.70"
	/*
	 * Ping of Death!
	 */
	if (icmp->icmp_type == ICMP_IPOD &&
	    icmp->icmp_code == ICMP_IPOD_IPOD &&
	    ntohs(ip->ip_len) == 666) {
#ifdef PODHOST
		struct in_addr pod_in;
		
		inet_aton(PODHOST, &pod_in);
		if (ip->ip_src.s_addr != pod_in.s_addr)
			return;
#endif
		panic("You got the POD!");
		return;
	}
#endif
}

