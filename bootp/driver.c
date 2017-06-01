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

#include <oskit/c/assert.h>
#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>
#include <oskit/c/strings.h>	/* bzero */
#include <oskit/c/arpa/inet.h>

#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/error.h>
#ifdef KNIT
#include <oskit/dev/dev.h>
#else 
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_timer.h>
#endif

#include <oskit/x86/proc_reg.h>

#include "bootp.h"


/*
 * This is used for timeouts on waiting for a bootp reply packet.
 */
static volatile int ticks;

static void
bumpticks()
{
	ticks++;
}

int
bootp_currticks()
{
	return ticks;
}


/* flags raised by net_receive indicating successful reply */
volatile int bootp_reply = 0;

/* This is the internal structure passed by the push routine */
struct etherdev {
	oskit_etherdev_t	*dev;
	oskit_netio_t	*send_nio;
	oskit_netio_t	*recv_nio;
	oskit_devinfo_t	info;
	unsigned char	haddr[OSKIT_ETHERDEV_ADDR_SIZE];
	struct bootp_net_info *bootp_info;
	struct bootp_t bootpreply;
};

static struct etherdev ed;

/*
 * This is our network receive callback.
 * We ignore all packets except for the BOOTP reply
 */
static oskit_error_t
net_receive(void *data, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	oskit_error_t rval = 0;
	unsigned char bcastaddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	static unsigned char *frame;
	struct ether_header *eth;
	int err;
	struct etherdev *dev = (struct etherdev *)data;

	struct bootp_t *bootpreply;
	struct  iphdr *ip;
	struct  udphdr *udp;

	if (pkt_size < ETH_MIN_PACKET || pkt_size > ETH_MAX_PACKET) {
		osenv_log(OSENV_LOG_ERR, PRF"%s: bad packet size %d\n",
			  dev->info.name, pkt_size);
		rval = OSKIT_E_DEV_BADPARAM;
		goto done;
	}

	err = oskit_bufio_map(b, (void **)&frame, 0, pkt_size);
	assert(err == 0);

	eth  = (struct ether_header *)frame;

	if (memcmp(eth->ether_dhost, &dev->haddr, sizeof dev->haddr) != 0
	    && memcmp(eth->ether_dhost, bcastaddr, sizeof bcastaddr) != 0) 
		goto done;			/* not for me */

	/* Make sure it is IP  */
	if ((pkt_size < (ETHER_HDR_SIZE + sizeof(struct iphdr) +
			sizeof(struct udphdr))) ||
	   (((frame[12] << 8) | frame[13]) != ETHERTYPE_IP))
		goto done;

	ip = (struct iphdr *)&frame[ETHER_HDR_SIZE];
	if ((ip->verhdrlen != 0x45) ||
		bootp_ipchksum(ip, sizeof(struct iphdr)) ||
		(ip->protocol != IP_UDP)) goto done;

	udp = (struct udphdr *)&frame[ETHER_HDR_SIZE + sizeof(struct iphdr)];

	/*
	 * Copy the entire reply (ip,udp,body) out and into a buffer
	 * that is known to be properly aligned for word access (not
	 * all machines are byte addressable like PCs are).
	 */
	bootpreply = &dev->bootpreply;
	memcpy(bootpreply, &frame[ETHER_HDR_SIZE], sizeof(struct bootp_t));

	if ((pkt_size >= (ETHER_HDR_SIZE + sizeof(struct bootp_t))) &&
	   (ntohs(udp->dest) == BOOTP_CLIENT) &&
	   (bootpreply->bp_op == BOOTP_REPLY)) {

		/* This is the packet we want */
		err = oskit_bufio_unmap(b, (void **)&frame, 0, pkt_size);
		assert(err == 0);

		/* We only decode the first packet we receive. */
		if (!bootp_reply) {
			memcpy(dev->bootp_info->server_hwaddr, 
				eth->ether_shost, sizeof eth->ether_shost);
			dev->bootp_info->flags |= BOOTP_NET_SERVER_ADDR;
			bootp_decode_packet(bootpreply, dev->bootp_info);
			bootp_reply = 1;  /* XXX: signal have the packet */
		}

	} /* else not BOOTP reply; just drop it */

 done:
	return rval;
}


/*
 * This sends a packet on the wire
 */
int
bootp_eth_transmit(char *src,		/* source ethernet address */
	     char *dest,		/* destination ethernet address */
	     unsigned short type,	/* ether type */
	     unsigned short size,
	     void *buf)
{
	oskit_bufio_t *b;
	oskit_error_t err = OSKIT_E_DEV_IOERR;
	int offset = 0;

	/* allocate a buffer to send */
	b = oskit_bufio_create(6+6+sizeof(type) + size);
	if (b != NULL) {
		oskit_size_t actual;

		/* fill in the ethernet packet */
		oskit_bufio_write(b, dest, offset, 6, &actual);
		assert(actual == 6);
		offset += 6;

		/* fill in source hardware address */
		oskit_bufio_write(b, src, offset, 6, &actual);
		assert(actual == 6);
		offset += 6;

		type = htons(type);
		oskit_bufio_write(b, &type, offset, sizeof(type), &actual);
		assert(actual == sizeof(type));
		offset += sizeof(type);

		oskit_bufio_write(b, buf, offset, size, &actual);
		assert(actual == size);
		offset += size;

		/* okay, `push' the packet onto the wire */
		err = oskit_netio_push(ed.send_nio, b, 6+6+sizeof(type) + size);

		oskit_bufio_release(b);
	}

	return err;
}


int
bootp(oskit_etherdev_t *trydev, struct bootp_net_info *info)
{
	return bootp_gen(trydev, info, BOOTP_MAX_RETRIES, BOOTP_TIMEOUT);
}

int
bootp_gen(oskit_etherdev_t *trydev, struct bootp_net_info *info, 
	int retry, int bootp_timeout)
{
	oskit_error_t err;
	int ret;
	unsigned long myip = 0;
#ifdef KNIT
#define oskit_osenv_timer_register(a,b,c) osenv_timer_register((b),(c))
#define oskit_osenv_timer_unregister(a,b,c) osenv_timer_unregister((b),(c))
#else
	oskit_osenv_t *osenv = 0;
	oskit_osenv_timer_t *osenv_timer = 0;

	/*
	 * Find the osenv, and then the osenv_timer interface.
	 */
	oskit_lookup_first(&oskit_osenv_iid, (void *) &osenv);
	assert(osenv);
	oskit_osenv_lookup(osenv, &oskit_osenv_timer_iid,(void *)&osenv_timer);

	assert(osenv_timer);
#endif
	oskit_osenv_timer_register(osenv_timer, bumpticks, BOOTP_TIMER_FREQ);

	/*
	 * first, check whether client knows its IP address
	 * this is the only member of bootpinfo that we honor
	 */
	if (info->flags & BOOTP_NET_IP)
		myip = ntohl(info->ip.s_addr);
		
	/* clear the structure */
	bzero(info, sizeof(*info));
	ed.bootp_info = info;

	/* trying this ethernet device */
	ed.dev = trydev;

	/* 
	 * keep a reference to the eth device for the duration of the bootp! 
	 */
	oskit_etherdev_addref(ed.dev);

	ed.recv_nio = oskit_netio_create(net_receive, &ed);
	if (ed.recv_nio == NULL)
		panic("unable to create recv_nio");
	oskit_etherdev_getaddr(ed.dev, ed.haddr);

	err = oskit_etherdev_getinfo(ed.dev, &ed.info);
	if (err)
		panic("error getting info from ethercard");

	/* Open it. */
	err = oskit_etherdev_open(ed.dev, 0, ed.recv_nio, &ed.send_nio);
	if (err)
		panic("Error opening ethercard");

	osenv_log(OSENV_LOG_NOTICE,
		  PRF"Trying bootp with the %s card (%x:%x:%x:%x:%x:%x)...\n",
		  ed.info.name,
		  ed.haddr[0], ed.haddr[1], ed.haddr[2],
		  ed.haddr[3], ed.haddr[4], ed.haddr[5]);

	ret = bootp_try(ed.haddr, myip, retry, bootp_timeout);
	if (ret == 0)
		osenv_log(OSENV_LOG_NOTICE, PRF"succeed\n");

	/* done with bootp, release resources */
	oskit_etherdev_release(ed.dev);
	oskit_netio_release(ed.send_nio);
	oskit_netio_release(ed.recv_nio);

	bootp_reply = 0;	/* XXX reset */

	oskit_osenv_timer_unregister(osenv_timer, bumpticks, BOOTP_TIMER_FREQ);

	return ret;
}
