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
 * This is totally experimental and only for people with an understanding
 * of the BSD networking stack.
 * 
 * Normally, the socket::write operation will create a new mbuf and
 * copy the data into it.
 * If one could construct (i.e. fake) an mbuf out of an application
 * buffer, we might save one copy.  This is what this program tries to do.
 * 
 * Unfortunately, it doesn't work because the networking stack insist
 * on forming an mbuf chain, even if the passed mbuf is big enough to hold
 * the TCP header.  See the #if notyet parts of tcp_output.c
 * (However, it might work if the BSD people ever overhaul their mbuf
 * system.)
 * Unfortunately, this will cause a copy later in the path because our
 * Linux device drivers only accept contiguous buffers.
 *
 * See also bufio_stream_recv.c
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
 * this is a simple ttcp-like client, programmed to the socket COM interface
 */
static void
sender(char *host, short port, int n, int mlen)
{
	oskit_socket_t 		*so;
	oskit_bufio_stream_t 	*oso;
	oskit_bufio_t 		*b;
	oskit_error_t 		err;
	int			i, k, namelen; 
	int			hlen = 128;

	struct  sockaddr_in 	addr;
	char			*data;

	if ((err = oskit_socket_factory_create(socket_create, OSKIT_AF_INET, 
			OSKIT_SOCK_STREAM, IPPROTO_TCP, &so))) {
		printf("socket_create: errno = %x\n", (int)err);
		return;
	}

	/* fill in the sin_addr structure */
	memset(&addr, 0, namelen = sizeof(addr));
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_family = OSKIT_AF_INET;
	addr.sin_port = htons(port);

	printf("connecting...\n");
	if ((err = oskit_socket_connect(so, 
		(struct oskit_sockaddr *)&addr, sizeof(addr)))) 
	{
		printf("connect: errno = 0x%x: %s\n", (int)err, strerror(err));
		return;
	}
	printf("connected to %s\n", host);

	if ((err = oskit_socket_query(so, &oskit_bufio_stream_iid, 
			(void **)&oso))) {
		printf("query: errno is 0x%x\n", (int)err);
		return;
	}

	for (k = 0; k < n; k++) {
	    b = oskit_bufio_create(hlen + mlen);
	    if (!b)
		panic("can't create bufio\n");
	    printf("creating bufio @%p\n", b);
    #if 0
	    oskit_bufio_map(b, (void **)&data, hlen, mlen); 
	    for (i = 0; i < mlen; i++)
		data[i] = i;
	    oskit_bufio_unmap(b, (void *)data, hlen, mlen); 
    #endif
	    oskit_bufio_stream_write(oso, b, hlen);
	    oskit_bufio_release(b);
	}

	oskit_bufio_stream_release(oso);
	oskit_socket_release(so);
	osenv_timer_spin(500000000);
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

	sender("155.99.214.159", 5001, 4, 1440); /* iffy */

	/* close etherdev and release net_io devices */
	oskit_freebsd_net_close_ether_if(eif);
	exit(0);
}
