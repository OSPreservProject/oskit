/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Simple test program for the DPF packet filter interface.
 *
 * It sets up a packet filter to let only ETHERTYPE_ARP packets get to
 * the receive function.
 */

#include <oskit/types.h>
#include <oskit/dpf.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <oskit/io/netio.h>
#include <oskit/io/bufio.h>
#include <oskit/io/dpf.h>

#include <oskit/dev/ethernet.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/net.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

#include <oskit/net/ether.h>

/* 
 * Used to pass the original PID returned from oskit_dpf_insert(), 
 * the currently matched DPF PID (set by the dpf_netio's callback),
 * and count of matched filters
 */
struct testdata {
        int orig_pid;
	int pid;
	int count;
};

#define DPF_COM_NUM_PACKETS 10

static oskit_error_t
filtered_recv(void *param, oskit_bufio_t *b, oskit_size_t pkt_size) 
{
	struct testdata *td= (struct testdata *) param;
	unsigned char *pkt;
	int i;

	oskit_bufio_map(b, (void **)&pkt, 0, pkt_size);
	printf("arp received from: ");

	for (i=0; i<4; i++)
		printf("%d.", pkt[28+i]);

	printf(" for: ");
	for (i=0; i<4; i++)
		printf("%d.", pkt[38+i]);

	printf("\n");

	if (td->pid == td->orig_pid) {
		printf(" Packet matched correct PID %d\n", td->pid);
	} else {
		printf(" Packet matched INCORRECT PID %d\n", td->pid);
	}		

	oskit_bufio_unmap(b, (void *)pkt, 0, pkt_size);

	/* increment count */
	if (++td->count == DPF_COM_NUM_PACKETS) {
		exit(0);
	}

	return 0;
}

void
main(void)
{
	int err;
	struct testdata td;
	void *filter;
	int filter_sz;
	int ndev;
	oskit_etherdev_t **etherdev;
	oskit_netio_t *filtered_netio, *device_push_netio, *dpf_netio;

	oskit_bufio_t *test_bufio;

	char ip_addr[8];
	char *test_pkt;

	td.pid = -1; 
	td.count = 0;

	oskit_clientos_init();
	start_net_devices();

	/*
	 * Find all the Ethernet device nodes.
	 */
	printf("looking up devices\n");
	ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&etherdev);
	if (ndev <= 0)
		panic("no Ethernet adaptors found!");

	printf("found %d devices\n", ndev);

	/* 
	 * Create a netio that calls filtered_recv() when a packet
	 * is pushed to it; &td is passed as a parameter. 
	 */
	filtered_netio = oskit_netio_create(filtered_recv, &td);

	/*
	 * Create a oskit_dpf_netio that will push filtered packets to
	 * filtered_netio. 
	 */
	oskit_netio_dpf_create(&td.pid, filtered_netio, &dpf_netio);

	/* 
	 * Create a DPF packet filter and insert it into system tree. 
	 */
	filter = oskit_dpf_mk_arp_req(&filter_sz, 0);
	td.orig_pid = oskit_dpf_insert(filter, filter_sz);

	/* 
	 * Create a fake packet and push it the netio 
	 * as a sanity check. 
	 */
	ip_addr[0] = 16;
	ip_addr[1] = 32;
	ip_addr[2] = 0;
	ip_addr[3] = 0;
	printf("created ip_addr for test packet\n");

#define PKT_SIZE 60
	test_bufio = oskit_bufio_create(PKT_SIZE);
	oskit_bufio_map(test_bufio, (void **)&test_pkt, 0, PKT_SIZE);
	oskit_dpf_mk_arp_req_string(test_pkt, ip_addr);
	oskit_netio_push(dpf_netio, test_bufio, PKT_SIZE);
	oskit_bufio_unmap(test_bufio, (void *)test_pkt, 0, PKT_SIZE);
	
	/* 
	 * Open the first ethernet device and instruct it to
	 * push packets to dpf_netio. Open initializes device_push_netio,
	 * but we don't use it because we don't push packets to
	 * the ethernet device in this example. 
	 */
	err = oskit_etherdev_open(etherdev[0], 0, dpf_netio,
		&device_push_netio);
	assert(!err);


	printf("Waiting for %d ARP packets\n", DPF_COM_NUM_PACKETS);

	/* Will exit in above code */
	while (1) {;}
}

