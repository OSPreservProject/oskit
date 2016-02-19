/*
 * Copyright (c) 1994-1997 The University of Utah and the Flux Group.
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

#include <oskit/types.h>
#include <oskit/dpf.h>
#include <oskit/pd.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>  /* bcopy */

#include <netinet/in.h>
#include <arpa/inet.h>

#include <oskit/io/netio.h>
#include <oskit/io/bufio.h>
#include <oskit/io/dpf.h>

#include <oskit/dev/dev.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/linux.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

#include <oskit/net/ether.h>

#include "bootp.h"
#include "netinet.h"

/* 
   Create a packet dispatcher that sends arp requests to one netio
   and arp replies to another. 
   
 */
volatile static int pkt_count = 0;

char *reqstr = "request";
char *repstr = "reply";

static oskit_error_t
recvd_unknown(void *param, oskit_bufio_t *b, oskit_size_t pkt_size) 
{
	printf("%d. Unrecognized packet.\n", ++pkt_count);
	return 0;
}

void
recvd_arp(oskit_bufio_t *b, oskit_size_t pkt_size, char *type) 
{
	unsigned char *pkt;
	int i;

	oskit_bufio_map(b, (void **)&pkt, 0, pkt_size);
	printf("%d. ARP %s received from: ", ++pkt_count, type);

	for (i=0; i<4; i++)
		printf("%d.", pkt[28+i]);
	printf(" for: ");
	for (i=0; i<4; i++)
		printf("%d.", pkt[38+i]);
	printf("\n");

	oskit_bufio_unmap(b, (void *)pkt, 0, pkt_size);
}

static oskit_error_t
recvd_arp_request(void *param, oskit_bufio_t *b, oskit_size_t pkt_size) 
{
	recvd_arp(b, pkt_size, reqstr);
	return 0;
}

static oskit_error_t
recvd_arp_reply(void *param, oskit_bufio_t *b, oskit_size_t pkt_size) 
{
	recvd_arp(b, pkt_size, repstr);
	return 0;
}

static void
whoami(oskit_etherdev_t *dev, struct in_addr *myip)
{
	char ipaddr[4*4];			/* nnn.nnn.nnn.nnnz */
	struct in_addr ip;

	get_ipinfo(dev, ipaddr, 0, 0, 0);
	inet_aton(ipaddr, &ip);
	/* no, just return network order */
	/* myip->s_addr = ntohl(ip.s_addr); */
	myip->s_addr = ip.s_addr;
}

void
main(void)
{
	oskit_etherdev_t **etherdev;
	oskit_netio_t *req_netio;
	oskit_netio_t *rep_netio;
	oskit_netio_t *default_netio;
	oskit_netio_t *ether_push_netio;
	oskit_netio_t *pd_push_netio; /* packet dispatcher's push */
	oskit_pd_t *pd;               /* packet dispatcher */
	oskit_s32_t reqfid;          /* FID for ARP receive  */
	oskit_s32_t repfid;          /* FID for ARP reply */
	oskit_u32_t fsz;              /* Filter size in bytes */
	oskit_s32_t ndev;
	oskit_u32_t err;
	oskit_u8_t my_eth_addr[6]; 
	struct in_addr my_ip;         /* 32-bit IP address, network order */
	struct in_addr dst_ip;        /* This will be marker's IP */
	void *filter;          
	int i;

	oskit_u8_t etherbroadcastaddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	oskit_bufio_t *arp_bufio;
	struct ether_header *eh;
	struct ether_arp *ea;
	char *frame;

	oskit_clientos_init();
	start_net_devices();

	/*
	 * Find all the Ethernet device nodes.
	 */
	printf("Looking up devices\n");
	ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&etherdev);
	if (ndev <= 0)
		panic("no Ethernet adaptors found!");

	printf("Found %d devices\n", ndev);

	/* 
	 * Create a netio that calls recvd_arp_request() when a packet
	 * is pushed to it.
	 */
	req_netio = oskit_netio_create(recvd_arp_request, 0);

	/* 
	 * Create a netio that calls recvd_arp_reply() when a packet
	 * is pushed to it.
	 */
	rep_netio = oskit_netio_create(recvd_arp_reply, 0);

	/* 
	 * Create the default netio channel.
	 */
	default_netio = oskit_netio_create(recvd_unknown, 0);

	/*
	 * Create the packet dispatcher.
	 */
	oskit_packet_dispatcher_create(&pd, 
				       OSKIT_PD_FILTER_DPF_INTERP,
				       &pd_push_netio,
				       default_netio);
	/* 
	 * Using the dpf template library, create filters
	 * that catches arp requests and replies. 
	 */
	filter = oskit_dpf_mk_arp_req(&fsz, 0);
	oskit_packet_dispatcher_register(pd, &reqfid, req_netio, filter, fsz);
	filter = oskit_dpf_mk_arp_rep(&fsz, 0);
	oskit_packet_dispatcher_register(pd, &repfid, rep_netio, filter, fsz);

#define PD_ARP_SZ 42

/* 
 * This clause for making sure that we differentiate between replies and
 * requests without having to receive a reply. We attempt below
 * to generate a reply by sending out a request from us, but sometimes
 * no one will answer the request because they don't know who we are.
 */
#if 0
	{
	oskit_bufio_t *test_bufio;
	void *test_pkt;
	char ip_addr[8];
	/*
	 * Test an ARP reply packet manually since we sometimes
	 * don't see them within a 10 packet session.
	 */
       
	test_bufio = oskit_bufio_create(PD_ARP_SZ);
	oskit_bufio_map(test_bufio, (void **)&test_pkt, 0, PD_ARP_SZ);
	oskit_dpf_mk_arp_rep_string(test_pkt, ip_addr);
	oskit_netio_push(pd_push_netio, test_bufio, PD_ARP_SZ);
	oskit_bufio_unmap(test_bufio, (void *)test_pkt, 0, PD_ARP_SZ);	

	}
#endif /* 0 or 1 */

	/* Create an ARP request in an attempt to get a reply. */

	/* Get the eth and IP address of our first interface */
	oskit_etherdev_getaddr(etherdev[0], my_eth_addr);
	whoami(etherdev[0], &my_ip);

	/* Create a bufio for the packet data and init the header structs */
	arp_bufio = oskit_bufio_create(PD_ARP_SZ);
	oskit_bufio_map(arp_bufio, (void **)&frame, 0, PD_ARP_SZ);
	eh = (struct ether_header *)frame;
	ea = (struct ether_arp *)(frame + sizeof(*eh));
	bzero((char*) ea, sizeof(*ea));

	/* ether_header fields */
	bcopy(etherbroadcastaddr, eh->ether_dhost, sizeof(eh->ether_dhost));
	bcopy(my_eth_addr, eh->ether_shost, sizeof(eh->ether_shost));
	eh->ether_type = htons(ETHERTYPE_ARP);

	/* ether_arp struct arphdr fields */
	ea->arp_hrd = htons(ARPHRD_ETHER);
	ea->arp_pro = htons(ETHERTYPE_IP);
	ea->arp_hln = sizeof(ea->arp_sha);
	ea->arp_pln = sizeof(ea->arp_spa);
	ea->arp_op = htons(ARPOP_REQUEST);

	bcopy(my_eth_addr, ea->arp_sha, sizeof(ea->arp_sha));
	bcopy(&my_ip.s_addr, &ea->arp_spa, sizeof(ea->arp_spa));
	inet_aton("155.99.212.61", &dst_ip);
	bcopy(&dst_ip.s_addr, &ea->arp_tpa, sizeof(ea->arp_tpa));

	/* Uncomment the next line if you want to test the packet integrity. 
	 * Can also do 'tcpdump arp' from any other machine on the
	 *  subnet. Should produce the following output:
	 *
	 * 14:41:27.780918 arp who-has marker.cs.utah.edu tell iffy.cs.utah.edu
	 *
	 * (the above is from this test.)
	 */
	/* oskit_netio_push(pd_push_netio, arp_bufio, PD_ARP_SZ); */

	/* 
	 * Open the first ethernet device and instruct it to
	 * push packets to the packet dispather. 
	 */
	printf("Opening ethernet device.\n");
	err = oskit_etherdev_open(etherdev[0], 0, pd_push_netio,
		&ether_push_netio);
	assert(!err);

#define PD_NUM_PACKETS 10
	printf("Waiting for %d ARP packets\n", PD_NUM_PACKETS);

	/* Send out homemade request */
	oskit_netio_push(ether_push_netio, arp_bufio, PD_ARP_SZ);

	while (pkt_count < PD_NUM_PACKETS) { ; }
	exit(pkt_count);
}





