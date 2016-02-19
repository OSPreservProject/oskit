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
 * The Ethernet layer.
 */
#include <oskit/dev/dev.h>
#include <oskit/dev/ethernet.h>
#include <oskit/io/bufio.h>
#include <oskit/io/netio.h>
#include <oskit/net/ether.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <arpa/inet.h>
#include <oskit/net/arp.h>

#include <oskit/udplib.h>
#include "udplib.h"

struct etherdev {
	oskit_etherdev_t	*dev;
	oskit_netio_t		*send_nio;
	oskit_netio_t		*recv_nio;
	oskit_devinfo_t		info;
	struct in_addr		ipaddr;		/* For ARP, in network order*/
	unsigned char		haddr[OSKIT_ETHERDEV_ADDR_SIZE];
};
static struct etherdev		ed;

static unsigned char	bcastaddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/*
 * XXX Kludgy. We should pass around an interface handle instead.
 *     Returns in host order.
 */
void
myipaddr(struct in_addr *ipaddr)
{
	ipaddr->s_addr = ntohl(ed.ipaddr.s_addr);
}

/*
 * This is our network receive callback.
 * We just add the buf to a queue that is emptied later.
 *
 * Returns zero on success, an error code from <oskit/error.h> otherwise.
 */
static oskit_error_t
net_receive(void *data, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	struct etherdev *dev = (struct etherdev *)data;
	struct ether_header	*eth;
	int			err;

	if (pkt_size < ETH_MIN_PACKET || pkt_size > ETH_MAX_PACKET) {
		printf("%s: bad packet size %d\n", dev->info.name, pkt_size);
		return OSKIT_E_DEV_BADPARAM;
	}

	/*
	 * Check the Ethernet header to see if it is addressed to us.
	 */
	err = oskit_bufio_map(b, (void **)&eth, 0, pkt_size);
	assert(err == 0);

	if (memcmp(eth->ether_dhost, &dev->haddr, sizeof dev->haddr) != 0
	    && memcmp(eth->ether_dhost, bcastaddr, sizeof bcastaddr) != 0)
		return 0;		/* not for us */


	/*
	 * Deal with ARP traffic.
	 */
	if (ntohs(eth->ether_type) == ETHERTYPE_ARP) {
		arp_interrupt(b, pkt_size);
		return 0;
	}

	/*
	 * Pass it up to find a socket that wants it.
	 */
	ip_interrupt(b, pkt_size);

	return 0;
}

/*
 * Open ethernet device and get ready for both sending and receiving.
 */
oskit_error_t
eth_initdev(oskit_etherdev_t *dev, struct in_addr ipaddr,
	    unsigned char out_addr[OSKIT_ETHERDEV_ADDR_SIZE])
{
	oskit_error_t	err;
	
	ed.dev = dev;
	ed.ipaddr.s_addr = ipaddr.s_addr;

	/*
	 * Fill in `ed' with info about this dev.
	 */
	ed.recv_nio = oskit_netio_create(net_receive, &ed);
	if (ed.recv_nio == NULL)
		panic("unable to create recv_nio\n");
	
	oskit_etherdev_getaddr(ed.dev, ed.haddr);

	/* XXX sizeof(out_addr) returns 4 on the arm!? */
	memcpy(out_addr, ed.haddr, OSKIT_ETHERDEV_ADDR_SIZE);

	err = oskit_etherdev_getinfo(ed.dev, &ed.info);
	if (err)
		panic("Error(0x%08x) getting info from ethercard", err);

	/*
	 * Open it.
	 */
	err = oskit_etherdev_open(ed.dev, 0, ed.recv_nio, &ed.send_nio);
	if (err)
		panic("Error(0x%08x) opening ethercard", err);

	return 0;
}

/*
 * Send an Ethernet frame.
 */
oskit_error_t
eth_transmit(unsigned char src[],
	     unsigned char dst[],
	     unsigned short type,
	     oskit_bufio_t *b,
	     oskit_size_t blen)
{
	oskit_error_t err;
	struct ether_header *eth;

	if (blen > ETH_MAX_PACKET)
		return OSKIT_E_INVALIDARG;

	/*
	 * Map in the bufio so `eth' points to the Ethernet header part.
	 */
	err = oskit_bufio_map(b, (void **)&eth, 0, blen);
	if (err)
		return err;

	/*
	 * Fill in the Ethernet header.
	 */
	if (src)
		memcpy(eth->ether_shost, src, ETHER_ADDR_SIZE);
	else
		memcpy(eth->ether_shost, ed.haddr, ETHER_ADDR_SIZE);
	
	memcpy(eth->ether_dhost, dst, ETHER_ADDR_SIZE);
	eth->ether_type = htons(type);

	oskit_bufio_unmap(b, eth, 0, blen);

	return oskit_netio_push(ed.send_nio, b, blen);
}

/*
 * Arp Code.
 */

/*
 * Lookup the Ethernet address for an IP address.
 */
oskit_error_t
arpresolve(const struct in_addr *ipaddr)
{
	int		  blen;
	oskit_bufio_t	 *b;
	struct ether_arp *arp;
	oskit_error_t	  err;

	/*
	 * Allocate the bufio.
	 * It contains the whole ethernet frame.
	 * The various layers below us fill in their respective parts.
	 */
	blen = sizeof(struct ether_header) + sizeof(struct ether_arp);
	b = oskit_bufio_create(blen);
	if (b == NULL)
		return OSKIT_E_OUTOFMEMORY;

	/*
	 * Map it in so `arp' points to the arp part.
	 */
	err = oskit_bufio_map(b, (void **)&arp,
			      sizeof(struct ether_header),
			      sizeof(struct ether_arp));
	if (err)
		goto done;

	arp->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
	arp->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
	arp->ea_hdr.ar_hln = ETHER_ADDR_SIZE;
	arp->ea_hdr.ar_pln = sizeof (struct in_addr);
	arp->ea_hdr.ar_op  = htons(ARPOP_REQUEST);

	/* Him */
	memset(arp->arp_tha, 0, ETHER_ADDR_SIZE);
	memcpy(arp->arp_tpa, ipaddr, sizeof (struct in_addr));
	/* Me */
	memcpy(arp->arp_sha, ed.haddr, ETHER_ADDR_SIZE);
	memcpy(arp->arp_spa, &ed.ipaddr, sizeof (struct in_addr));
	
	/* Goes to the broadcast address on the outgoing interface. */
	err = eth_transmit(ed.haddr, bcastaddr, ETHERTYPE_ARP, b, blen);

	/* Just delay for a bit. */
	osenv_timer_spin(100000000);

 done:
	oskit_bufio_release(b);
	return err;
}

void
arpreply(struct in_addr *tip, unsigned char teth[ETHER_ADDR_SIZE])
{
	int		  blen;
	oskit_bufio_t	 *b;
	struct ether_arp *arp;
	oskit_error_t	  err;

	/*
	 * Allocate the bufio.
	 * It contains the whole ethernet frame.
	 * The various layers below us fill in their respective parts.
	 */
	blen = sizeof(struct ether_header) + sizeof(struct ether_arp);
	b = oskit_bufio_create(blen);
	if (b == NULL)
		return;

	/*
	 * Map it in so `arp' points to the arp part.
	 */
	err = oskit_bufio_map(b, (void **)&arp,
			      sizeof(struct ether_header),
			      sizeof(struct ether_arp));
	if (err) {
		oskit_bufio_release(b);
		return;
	}

	arp->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
	arp->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
	arp->ea_hdr.ar_hln = ETHER_ADDR_SIZE;
	arp->ea_hdr.ar_pln = sizeof (struct in_addr);
	arp->ea_hdr.ar_op  = htons(ARPOP_REQUEST);

	/* Him */
	memcpy(arp->arp_tha, teth, ETHER_ADDR_SIZE);
	memcpy(arp->arp_tpa, tip, sizeof (struct in_addr));
	/* Me */
	memcpy(arp->arp_sha, ed.haddr, ETHER_ADDR_SIZE);
	memcpy(arp->arp_spa, &ed.ipaddr, sizeof (struct in_addr));
	
	/* Send it */
	(void) eth_transmit(ed.haddr, teth, ETHERTYPE_ARP, b, blen);
	oskit_bufio_release(b);
}

oskit_error_t
arp_interrupt(oskit_bufio_t *b, oskit_size_t pkt_size)
{
	struct ether_header	*eth;
	struct ether_arp	*arp;
	struct in_addr		sip;
	oskit_error_t		err;

	err = oskit_bufio_map(b, (void **)&eth, 0, pkt_size);
	assert(err == 0);

	arp = (struct ether_arp *)((void *)eth + ETHER_HDR_SIZE);
	memcpy(&sip, &arp->arp_spa, sizeof(sip));

	farp_add(&sip, (const char *)arp->arp_sha);

	/* respond to ARP requests for us */
	if (arp->ea_hdr.ar_hrd == htons(ARPHRD_ETHER) &&
	    arp->ea_hdr.ar_pro == htons(ETHERTYPE_IP) &&
	    arp->ea_hdr.ar_op == htons(ARPOP_REQUEST) &&
	    memcmp(&arp->arp_tpa, &ed.ipaddr, sizeof(struct in_addr)) == 0)
		arpreply(&sip, arp->arp_sha);

	return 0;
}

