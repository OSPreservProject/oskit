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
 * Simple test program for the SPF packet filter interface.
 *
 * It sets up a packet filter to let only ETHERTYPE_ARP packets get to
 * the receive function.
 */

#include <oskit/types.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <oskit/io/netio.h>
#include <oskit/io/bufio.h>
#include <oskit/io/spf.h>

#include <oskit/dev/ethernet.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/net.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

#include <oskit/net/ether.h>

static oskit_error_t
filtered_recv(void *param, oskit_bufio_t *b, oskit_size_t pkt_size)
{
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

	oskit_bufio_unmap(b, (void *)pkt, 0, pkt_size);

	/* increment count */
	return ++(*(int *)param);
}


/*
 * This is a silly example.  It just waits until the packet
 * filter it has installed receives 10 packets -- ie, it has
 * received 10 ARP requests.
 */
void
main(void)
{
	int err;
	volatile int count = 0;
	oskit_netio_t *filtered_netio, *device_push_netio, *push_netio;

	oskit_etherdev_t **etherdev;
	int ndev;

	oskit_clientos_init();
	start_net_devices();

	printf("looking up devices\n");
	/*
	 * Find all the Ethernet device nodes.
	 */
	ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&etherdev);
	if (ndev <= 0)
		panic("no Ethernet adaptors found!");

	printf("found %d devices\n", ndev);


	/*
	 * Create a netio that calls my function when it gets packets
	 */
	filtered_netio = oskit_netio_create(filtered_recv, &count);

	/*
	 * Create a SPF that receives packets from push_netio and
	 * forwards them to filtered_netio if they match the params
	 * it initializes push_netio
	 */
	oskit_netio_spf_ethertype_create(ETHERTYPE_ARP, filtered_netio,
		&push_netio);
	/*
	 * Open the (first) ethernet device, with push_netio as
	 * the receiver; get the device send netio device_push_netio
	 * Open initializes device_push_netio, but we don't use it.
	 */
	err = oskit_etherdev_open(etherdev[0], 0, push_netio,
		&device_push_netio);
	assert(!err);

	printf("Waiting for 10 ARP packets\n");

	while (count < 10)
		;
}

