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
 * A simple example for the netio->openfile adapter.
 *
 * It creates the openfile object, waits 20 secs, then reads 10 packets.
 */

#include <oskit/types.h>
#include <oskit/io/netio.h>
#include <oskit/com/listener.h>
#include <oskit/clientos.h>
#include <oskit/fs/openfile.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/linux.h>
#include <oskit/time.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/soa.h>
#include <oskit/startup.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * perform the netio exchange
 */
static oskit_netio_t *
exchange(void *arg, oskit_netio_t *recv_nio)
{
	oskit_etherdev_t	*edev = arg;
	oskit_netio_t	*send_nio;
	oskit_error_t	err = oskit_etherdev_open(edev, 0, recv_nio,
		&send_nio);

	return err ? 0 : send_nio;
}

/*
 * a packet is being dumped (q points to the oskit_bufio_t of the packet)
 */
static oskit_error_t
packet_dumped(struct oskit_iunknown *q, void *arg)
{
	int *dropped = arg;
	++*dropped;
	printf("%dth packet is being dropped\n", *dropped);
	return 0;
}

void
main(void)
{
	int count = 0;
	int ndev;
	struct oskit_openfile	*of;
	oskit_etherdev_t **etherdev;
	int dropped = 0;
	oskit_listener_t *dumped;
	char 	buf[2000];

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

	dumped = oskit_create_listener(packet_dumped, &dropped);

	create_openfile_from_netio(
			exchange, 	/* open callback */
			etherdev[0], 	/* callback arg */
			5, 		/* queue length */
			OSKIT_O_RDWR, 	/* open flags */
			dumped,		/* packet is being dumped */
			(oskit_file_t *)0, 	/* associated oskit_file */
			&of);
	oskit_listener_release(dumped);

	printf("I will wait for 20 seconds before I start reading packets\n");
	printf("If you want to see me drop packets, send some\n");
	{
		oskit_clock_t *clock = oskit_clock_init();
		volatile oskit_timespec_t ts0, ts1;

		oskit_clock_gettime(clock, (oskit_timespec_t *)&ts0);
		do {
			oskit_clock_gettime(clock, (oskit_timespec_t *)&ts1);
		} while (ts1.tv_sec - ts0.tv_sec < 20);
		oskit_clock_release(clock);
	}
	printf("Now reading 10 packets\n");

	while (count < 10) {
		int  r;
		oskit_openfile_read(of, buf, sizeof buf, &r);
		hexdumpb(0, buf, r);
		count++;
	}
	oskit_openfile_release(of);
	printf("Done, exiting\n");
}

