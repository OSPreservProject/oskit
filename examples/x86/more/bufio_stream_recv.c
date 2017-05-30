/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * This test is totally experimental, and only for people with a thorough
 * understanding of the freebsd networking stack.
 *
 * The idea is as follows: socket::read will in the BSD implementation
 * do a soreceive into an mbuf, which is then copied into the receive
 * buffer.
 *
 * A bufio stream allows you to "receive" a bufio from a socket, which is
 * directly mapped to the underlying mbuf.  This will save one copy on the
 * receive path.  In fact, you'll get access to the very same bufio that 
 * the device driver pushed to the networking stack.  Of course, it only
 * works in the absence of fragmentation, so restrict the sending packet
 * size to 1440.  It should work with a normal ttcp client.
 *
 * See also freebsd/net/socketcomio.c
 */
#include <oskit/dev/dev.h>
#include <oskit/dev/error.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/freebsd.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/timer.h>
#include <oskit/com/listener.h>
#include <oskit/io/bufio_stream.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/net/freebsd.h>
#include <oskit/net/socket.h>

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "bootp.h"

/* MST: Salt Lake City, UT */
long secondswest = 6 * 60 * 60;
#define LOCAL_TO_GMT(t) (t)->tv_sec += secondswest;

char IPADDR[20];
char GATEWAY[20];
char NETMASK[20];

/* This doesn't really matter; it's for the ifconfig hack */
#define IFNAME		"eth0"

/* variable to hold the socket creator function */
static oskit_socket_factory_t *socket_create;

/*
 * this is a simple ttcp-like server, programmed to the socket COM interface
 */
static void
receiver(short port, int n)
{
	oskit_socket_t 		*so, *lso;
	oskit_bufio_stream_t 	*oso;
	oskit_stream_t 		*ost;
	oskit_bufio_t 		*b;
	oskit_error_t 		err;
	oskit_clock_t    	*clock = oskit_clock_init();
	oskit_timespec_t  	starttime, endtime;
	int			br, namelen; 
	int			mlen = 1420;

	struct  sockaddr_in 	addr;
	char			*data;

	if ((err = oskit_socket_factory_create(socket_create, OSKIT_AF_INET, 
			OSKIT_SOCK_STREAM, IPPROTO_TCP, &lso))) {
		printf("socket_create: errno = %x\n", (int)err);
		return;
	}

	/* fill in the sin_addr structure */
	memset(&addr, 0, namelen = sizeof(addr));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = OSKIT_AF_INET;
	addr.sin_port = htons(port);

	if ((err = oskit_socket_bind(lso, 
		(struct oskit_sockaddr *)&addr, sizeof(addr)))) 
	{
		printf("bind: errno = 0x%x: %s\n", (int)err, strerror(err));
		return;
	}

	if ((err = oskit_socket_listen(lso, 5)))
	{
		printf("listen: errno = 0x%x: %s\n", (int)err, strerror(err));
		return;
	}

	printf("accepting clients\n");
	if ((err = oskit_socket_accept(lso, 
		(struct oskit_sockaddr *)&addr, &namelen, &so)))
	{
		printf("accept: errno = 0x%x: %s\n", (int)err, strerror(err));
		return;
	}

	printf("accepted client, reading data...\n");
	if ((err = oskit_socket_query(so, &oskit_bufio_stream_iid, 
			(void **)&oso))) {
		printf("query: errno is 0x%x\n", (int)err);
		return;
	}

	if ((err = oskit_socket_query(so, &oskit_stream_iid, 
			(void **)&ost))) {
		printf("query: errno is 0x%x\n", (int)err);
		return;
	}

        oskit_clock_gettime(clock, &starttime);

	br = 0;
	for (;;) {
#if 0
	    /* read */
	    oskit_bufio_stream_read(oso, &b, &mlen);
	    if (mlen) {
	#if 0
		    oskit_bufio_getsize(b, &mlen);
		    printf("read %d bytes\n", mlen);
		    oskit_bufio_map(b, (void **)&data, 0, mlen); 
		    hexdumpb(0, data, mlen);
		    oskit_bufio_unmap(b, (void *)data, 0, mlen); 
	#endif
		    /* and discard */
		    oskit_bufio_release(b);
	    }

#else
	    char buf[1440];
	    err = oskit_stream_read(ost, buf, sizeof buf, &mlen);
#endif
	    if (mlen == 0)
		break;
	    br += mlen;
	    /* printf("recv %d\n", mlen); */
	}
        oskit_clock_gettime(clock, &endtime);

	oskit_stream_release(ost);
	oskit_bufio_stream_release(oso);
	oskit_socket_release(so);
	oskit_socket_release(lso);
	osenv_timer_spin(500000000);
	{
	    double sec = (endtime.tv_sec - starttime.tv_sec) + 
		(endtime.tv_nsec - starttime.tv_nsec) / 1000000000.0;
	    double r = br * 8.0 / 1024.0 / 1024.0 / sec;
	    printf("read %d bytes in %.2fs, or %.2f MBit/s\n", 
		br, sec, r);
	}
}

/*
 * the initialization follows the ping_reply example...
 */
int
main(int argc, char **argv)
{
	struct oskit_freebsd_net_ether_if *eif;
	oskit_etherdev_t **etherdev;
	oskit_etherdev_t *dev;
	int err, ndev;

	printf("Initializing devices...\n");
	oskit_dev_init();
	oskit_linux_init_ethernet_tulip();

	printf("Probing devices...\n");
	oskit_dev_probe();
	oskit_dump_devices();

	/* initialize network code */
	err = oskit_freebsd_net_init(&socket_create);	
	assert(!err);

	/*
	 * Find all the Ethernet device nodes.
	 */
	ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&etherdev);
	if (ndev <= 0)
		panic("no Ethernet adaptors found!");

	/*
	 * for now, we just go for the first device...
	 */
	dev = etherdev[0];

	/* get the IP address & other info */
	get_ipinfo(dev, IPADDR, GATEWAY, NETMASK, &secondswest);

	/*
	 * Open first one; could use oskit_freebsd_net_open_first_ether_if,
	 * but we already have a handle on that one.
	 */
	err = oskit_freebsd_net_open_ether_if(dev, &eif);
	assert(!err);

	/* configure the interface */
	err = oskit_freebsd_net_ifconfig(eif, IFNAME, IPADDR, NETMASK);
	assert(!err);

	err = oskit_freebsd_net_add_default_route(GATEWAY);
	if (err)
		printf("couldn't add default route (%d)\n", err);

	receiver(5001, 1);	

	/* close etherdev and release net_io devices */
	oskit_freebsd_net_close_ether_if(eif);
	exit(0);
}
