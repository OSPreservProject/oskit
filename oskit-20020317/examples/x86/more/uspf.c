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
 * Example program showing how to implement arbitrary unsafe packet filters
 */

#include <oskit/types.h>

#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/stdio.h>
#include <oskit/c/assert.h>
#include <oskit/c/arpa/inet.h>

#include <oskit/io/netio.h>
#include <oskit/io/uspf.h>
#include <oskit/io/bufio.h>

#include <oskit/dev/dev.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/net.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

#include <oskit/net/ether.h>

/*
 * show received packets
 */
static oskit_error_t
receive(void *param, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	printf("broadcast packet received\n");

	/* increment count */
	return ++(*(int *)param);
}

/*
 * is this a broadcast packet?
 */
static oskit_error_t
is_broadcast(void *frame, oskit_size_t pkt_size)
{
	unsigned char br[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	return !memcmp(frame, br, sizeof br);
}

void
main(void)
{
	int err;
	volatile int count = 0;
	oskit_netio_t *filter_netio, *incoming_netio;
	oskit_netio_t *device_push_netio;
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

	/* create the netio which is to receive the filtered packets */
	incoming_netio = oskit_netio_create(receive, &count);

	/* create the unsafe packet filter */
	filter_netio = oskit_netio_uspf_create(is_broadcast, incoming_netio);

	/*
	 * Open the ethernet device, with the filter as the receiver
	 * get the device send netio device_push_netio
	 * it initializes device_push_netio
	 */
	err = oskit_etherdev_open(etherdev[0], 0, filter_netio,
		&device_push_netio);
	assert(!err);

	printf("Waiting for 10 broadcast packets\n");

	while (count < 10)
		;
}

