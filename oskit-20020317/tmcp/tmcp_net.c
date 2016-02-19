/*
 * Copyright (c) 1998-2001 The University of Utah and the Flux Group.
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
 * emulab.net "Testbed Master Control" protocol support.
 *
 * This file is a mess, it is a hodge-podge of stuff thrown together,
 * most of which exists elsewhere.
 */

#define DUMPTMCPINFO	/* dump received info to console */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <oskit/error.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/dev.h>
#include <oskit/io/netio.h>
#include <oskit/net/ether.h>
#include <oskit/net/arp.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <oskit/tmcp.h>
#include "tmcp.h"

static unsigned char src_ethaddr[ETHER_ADDR_SIZE];
static struct in_addr src_ip;
static unsigned short src_port;
static unsigned char dst_ethaddr[ETHER_ADDR_SIZE];
static struct in_addr dst_ip;
static unsigned short dst_port;
static short ipid;
static oskit_etherdev_t *etherdev;

static oskit_netio_t *send_nio, *recv_nio;
static oskit_netio_t *app_send_nio;
static volatile int waiting;
static char *msg;
static int msglen;

static void tmcp_handler(void *data, int plen);
static oskit_error_t tmcp_receive(void *data, oskit_bufio_t *b,
				  oskit_size_t psize);
static void arp_handler(void *pdata, struct in_addr *myip, unsigned char *myeth,
			oskit_netio_t *nio);
static int arp_request(struct in_addr *dstip, unsigned char *dsteth,
		       struct in_addr *myip, unsigned char *myeth,
		       oskit_netio_t *nio, int waittime);
static void arp_reply(void *pdata, unsigned long myip, unsigned char *myeth,
		      oskit_netio_t *nio);
static unsigned short ipcksum(void *ipv, int len);
static void swapn(unsigned char a[], unsigned char b[], int n);


/*
 * XXX Hack public interface XXX
 *
 * Allows us to use the application's netio in the event we are shutdown
 * while it is still open.
 */
void
tmcp_setsendnetio(oskit_netio_t *nio)
{
	oskit_netio_addref(nio);
	app_send_nio = nio;
}


int
tmcp_netstart(oskit_etherdev_t *edev, int myif, struct in_addr *myip,
	      struct in_addr *nextip, struct in_addr *tmcp_ip, int tmcp_port)
{
	int err;

	/*
	 * Record our info
	 */
	src_ip = *myip;
	oskit_etherdev_getaddr(edev, src_ethaddr);
	src_port = htons(tmcp_port);

	/*
	 * Open interface and setup netios
	 */
	recv_nio = oskit_netio_create(tmcp_receive, 0);
	if (recv_nio == 0)
		return OSKIT_ENOMEM;

	err = oskit_etherdev_open(edev, 0, recv_nio, &send_nio);
	if (err != 0) {
		oskit_netio_release(recv_nio);
		return err;
	}

	/*
	 * Setup server info
	 */
	dst_ip = *tmcp_ip;
	err = arp_request(nextip, dst_ethaddr, &src_ip, src_ethaddr,
			  send_nio, 0);
	if (err) {
		oskit_netio_release(send_nio);
		oskit_netio_release(recv_nio);
		return err;
	}
	dst_port = htons(tmcp_port);

	etherdev = edev;
	oskit_etherdev_addref(edev);

	return 0;
}

int
tmcp_netrestart(void)
{
	int err;

	if (etherdev == 0)
		return OSKIT_EINVAL;

	assert(send_nio == 0);
	err = oskit_etherdev_open(etherdev, 0, 0, &send_nio);
	if (err != 0) {
		/*
		 * If the user registered a netio, use that.
		 */
		if (app_send_nio != 0) {
			send_nio = app_send_nio;
			err = 0;
		}
	}

	return err;
}

void
tmcp_netstop(void)
{
	if (send_nio != 0) {
		oskit_netio_release(send_nio);
		send_nio = 0;
	}
	if (recv_nio != 0) {
		oskit_netio_release(recv_nio);
		recv_nio = 0;
	}
}

void
tmcp_netshutdown(void)
{
	if (send_nio != 0)
		oskit_netio_release(send_nio);
	if (recv_nio != 0)
		oskit_netio_release(recv_nio);
	if (app_send_nio)
		oskit_netio_release(app_send_nio);
	if (etherdev != 0)
		oskit_etherdev_release(etherdev);
}

/*
 * Cobble up a UDP message to send to the TMCD and wait for a reply.
 * Entry: buf contains the message to send, *buflen the size of the buffer.
 * Exit: buf contains the reply, *buflen the size of the reply.
 */
int
tmcp_sendmsg(char *buf, int *buflen)
{
	oskit_bufio_t *bio;
	oskit_error_t err;
	int plen, dlen, i;
	void *data, *p;
	struct ether_header eth;
	struct ip ip;
	struct udphdr udp;

	dlen = strlen(buf) + 1;
	plen = ETHER_HDR_SIZE + sizeof(ip) + sizeof(udp) + dlen;
	if (plen > ETH_MAX_PACKET)
		return OSKIT_EMSGSIZE;

	/*
	 * Allocate and map the packet bufio
	 */
	err = oskit_netio_alloc_bufio(send_nio, plen, &bio);
	if (err)
		return OSKIT_ENOMEM;
	err = oskit_bufio_map(bio, (void **)&data, 0, plen);
	if (err) {
		oskit_bufio_release(bio);
		return err;
	}

	/*
	 * Setup UDP Header
	 */
	udp.uh_sport = src_port;
	udp.uh_dport = dst_port;
	udp.uh_ulen = htons(sizeof(udp) + dlen);
	udp.uh_sum = 0;

	/*
	 * Setup IP header
	 */
	ip.ip_v = IPVERSION;
	ip.ip_hl = (sizeof ip) >> 2;
	ip.ip_tos = 0;
	ip.ip_len = htons(sizeof(ip) + sizeof(udp) + dlen);
	ip.ip_id = ++ipid;
	ip.ip_off = 0;
	ip.ip_ttl = 64;
	ip.ip_p = IPPROTO_UDP;
	ip.ip_src.s_addr = src_ip.s_addr;
	ip.ip_dst.s_addr = dst_ip.s_addr;
	ip.ip_sum = 0;
	ip.ip_sum = ipcksum(&ip, sizeof(ip));

	/*
	 * Set up ether header
	 */
	memcpy(eth.ether_shost, src_ethaddr, ETHER_ADDR_SIZE);
	memcpy(eth.ether_dhost, dst_ethaddr, ETHER_ADDR_SIZE);
	eth.ether_type = htons(ETHERTYPE_IP);

	/*
	 * Now copy into packet to avoid alignment breakage on ARM32
	 */
	p = data;
	memcpy(p, &eth, ETHER_HDR_SIZE);
	p += ETHER_HDR_SIZE;
	memcpy(p, &ip, sizeof(ip));
	p += sizeof(ip);
	memcpy(p, &udp, sizeof(udp));
	p += sizeof(udp);
	memcpy(p, buf, dlen);

	/* XXX UDP checksum */

	/*
	 * Unmap the packet and send it
	 * If not expecting a reply, just fire and forget. 
	 */
	oskit_bufio_unmap(bio, data, 0, plen);
	if (*buflen == 0) {
		err = oskit_netio_push(send_nio, bio, plen);
		oskit_bufio_release(bio);
		return err;
	}

	msg = buf;
	msglen = *buflen;
	waiting = 1;
	err = oskit_netio_push(send_nio, bio, plen);
	oskit_bufio_release(bio);
	if (err)
		return err;

	/*
	 * Wait awhile for the reply
	 */
	for (i = 0; i < 50; i++) {
		if (waiting == 0) {
			*buflen = msglen;
			return 0;
		}
		osenv_timer_spin(100000000); /* 100ms */
	}

	return OSKIT_ETIMEDOUT;
}

static oskit_error_t
tmcp_receive(void *data, oskit_bufio_t *b, oskit_size_t psize)
{
	oskit_error_t		err;
	struct ether_header	*eth;

	err = oskit_bufio_map(b, (void **)&eth, 0, psize);
	assert(err == 0);

	/*
	 * Deal with ARP traffic.
	 */
	if (eth->ether_type == htons(ETHERTYPE_ARP))
		arp_handler(eth, &src_ip, src_ethaddr, send_nio);
	else
		tmcp_handler(eth, psize);

	oskit_bufio_unmap(b, eth, 0, psize);
	return 0;
}

/*
 * Interrupt handler for TMCP packets
 */
static void
tmcp_handler(void *data, int plen)
{
	struct ether_header *eth;
	struct {
		struct ip ip;
		struct udphdr udp;
	} hdr;
	int len;

	if (!waiting)
		return;

	/*
	 * Basic checks:
	 *	- long enough to hold ETH/IP/UDP
	 *	- ethtype == ETHERTYPE_IP
	 */
	eth = data;
	if (plen < ETHER_HDR_SIZE + sizeof(hdr) ||
	    eth->ether_type != htons(ETHERTYPE_IP))
		return;

	/*
	 * XXX copy IP/UDP info to avoid alignment problems
	 */
	memcpy(&hdr, data + ETHER_HDR_SIZE, sizeof(hdr));

	/*
	 * IP checks:
	 *	- checksums
	 *	- IP version ok
	 *	- proto == IPPROTO_UDP
	 *	- from master, to us
	 */
	if (ipcksum(&hdr.ip, hdr.ip.ip_hl << 2) != 0 ||
	    hdr.ip.ip_v != IPVERSION ||
	    hdr.ip.ip_hl != sizeof(hdr.ip) >> 2 ||
	    hdr.ip.ip_p != IPPROTO_UDP ||
	    hdr.ip.ip_src.s_addr != dst_ip.s_addr ||
	    hdr.ip.ip_dst.s_addr != src_ip.s_addr)
		return;

	/*
	 * UDP checks:
	 *	- dst port correct
	 *	- length reasonable
	 */
	if (hdr.udp.uh_dport != src_port ||
	    ntohs(hdr.udp.uh_ulen) < sizeof(hdr.udp))
		return;

	/*
	 * Finally! Copy the data.
	 */
	len = ntohs(hdr.udp.uh_ulen) - sizeof(hdr.udp);
	if (len < msglen)
		msglen = len;
	memcpy(msg, data + ETHER_HDR_SIZE + sizeof(hdr), msglen);
	waiting = 0;
}


/*
 * ARP handling
 */
static volatile int arp_pending;
static struct ether_arp *arp_current;

static void
arp_handler(void *pdata, struct in_addr *myip, unsigned char *myeth,
	    oskit_netio_t *nio)
{
	struct ether_arp	*arp;
	unsigned long		ipaddr;

	arp = (struct ether_arp *)(pdata + ETHER_HDR_SIZE);
	ipaddr = *(unsigned long *)arp->arp_tpa;

	if (arp->arp_hrd != htons(ARPHRD_ETHER) ||
	    arp->arp_pro != htons(ETHERTYPE_IP) ||
	    ipaddr != myip->s_addr)
		return;

	if (ntohs(arp->arp_op) == ARPOP_REQUEST)
		arp_reply(pdata, myip->s_addr, myeth, nio);
	else if (arp_pending &&
		 !memcmp(arp_current->arp_tpa, arp->arp_spa, sizeof(*myip))) {
		memcpy(arp_current->arp_tha, arp->arp_sha, ETHER_ADDR_SIZE);
		arp_pending = 0;
	}
}

static int
arp_request(struct in_addr *dstip, unsigned char *dsteth,
	    struct in_addr *myip, unsigned char *myeth,
	    oskit_netio_t *nio, int waittime)
{
	struct ether_header	*eth;
	struct ether_arp	*arp;
	oskit_bufio_t		*our_buf;
	int			err, len;

 again:
	len = ETHER_HDR_SIZE + sizeof(*arp);
 	our_buf = oskit_bufio_create(len);
	if (our_buf == 0)
		return OSKIT_ENOMEM;

	err = oskit_bufio_map(our_buf, (void **)&eth, 0, len);
	if (err != 0) {
		oskit_bufio_release(our_buf);
		return err;
	}

	arp = (void *)eth + ETHER_HDR_SIZE;
	arp->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
	arp->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
	arp->ea_hdr.ar_hln = ETHER_ADDR_SIZE;
	arp->ea_hdr.ar_pln = sizeof (struct in_addr);
	arp->ea_hdr.ar_op  = htons(ARPOP_REQUEST);

	/* Him */
	memset(arp->arp_tha, 0, ETHER_ADDR_SIZE);
	memcpy(arp->arp_tpa, dstip, sizeof(struct in_addr));
	/* Me */
	memcpy(arp->arp_sha, myeth, ETHER_ADDR_SIZE);
	memcpy(arp->arp_spa, myip, sizeof (struct in_addr));

	/* Fill in the ethernet info. */
	memset(&eth->ether_dhost, -1, ETHER_ADDR_SIZE);
	memcpy(&eth->ether_shost, myeth, ETHER_ADDR_SIZE);
	eth->ether_type = htons(ETHERTYPE_ARP);

	arp_current = arp;
	arp_pending = 1;
	err = oskit_netio_push(nio, our_buf, len);
	if (err == 0) {
		int i;

		for (i = 0; i < 10; i++) {
			if (arp_pending == 0) {
				memcpy(dsteth, arp->arp_tha, ETHER_ADDR_SIZE);
				oskit_bufio_release(our_buf);
				return 0;
			}
			osenv_timer_spin(100000000); /* 100ms */
		}
		if (--waittime > 0) {
			oskit_bufio_release(our_buf);
			goto again;
		}
		err = OSKIT_ETIMEDOUT;
	}
	oskit_bufio_release(our_buf);
	return err;
}

/*
 * Reply to arp requests for this node. Lifted from examples/x86/pingreply.c
 */
static void
arp_reply(void *pdata, unsigned long myip, unsigned char *myeth,
	  oskit_netio_t *nio)
{
	struct ether_header	*eth = pdata;
	struct ether_arp	*arp;
	struct in_addr		ip;
	oskit_bufio_t		*our_buf;

	arp = (struct ether_arp *)(pdata + ETHER_HDR_SIZE);
	ip.s_addr = *(unsigned long *)arp->arp_tpa;

	if (ntohs(arp->arp_hrd) != ARPHRD_ETHER
	    || ntohs(arp->arp_pro) != ETHERTYPE_IP
	    || ntohs(arp->arp_op) != ARPOP_REQUEST
	    || ip.s_addr != myip)
		return;			/* wrong proto or addr */

	/* Send the reply. */
	arp->arp_op = htons(ARPOP_REPLY);
	swapn(arp->arp_spa, arp->arp_tpa, sizeof arp->arp_tpa);
	swapn(arp->arp_sha, arp->arp_tha, sizeof arp->arp_tha);
	memcpy(arp->arp_sha, myeth, 6);

	/* Fill in the ethernet addresses. */
	bcopy(&eth->ether_shost, &eth->ether_dhost, 6);
	bcopy(myeth, &eth->ether_shost, 6);

 	our_buf = oskit_bufio_create(sizeof(*eth) + sizeof(*arp));
	if (our_buf != NULL) {
		oskit_size_t got;

		oskit_bufio_write(our_buf, eth, 0, sizeof(*eth), &got);
		oskit_bufio_write(our_buf, arp,
				  sizeof(*eth), sizeof(*arp), &got);
		oskit_netio_push(nio, our_buf, sizeof(*eth) + sizeof(*arp));
		oskit_bufio_release(our_buf);
	} else {
		printf("couldn't allocate bufio for ARP reply\n");
		return;
	}
}

/*
 * Utility functions
 */

/*
 * IP checksum
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

static void
swapn(unsigned char a[], unsigned char b[], int n)
{
	int i;

	for (i = 0; i < n; i++) {
		unsigned char t;
		t = a[i];
		a[i] = b[i];
		b[i] = t;
	}
}
