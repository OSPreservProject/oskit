/*
 * Copyright (c) 1998, 1999 The University of Utah and the Flux Group.
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
 * An example showing how to use the FUDP (Fake UDP) library.
 * Sends the contents of the `msg' string to `DST_PORT' on `DST_IP'
 * via the FUDP library.
 * The other machine should be running the `fudp_recv' example to
 * receive the message.
 *
 * Be sure to configure the first part of this file appropriately.
 */

/*
 * Configure appropriately.
 */
const char *msg = "The OSKit FUDP example works.";
#define SRC_PORT 10000			/* must match fudp_recv.c */
#define DST_PORT 5050			/* must match arg to fudp_recv */
#define SRC_IP "155.99.212.51"				/* pencil */
#define DST_IP "155.99.212.96"				/* cornish */
#define DST_ETHER {0x0, 0xa0, 0xc9, 0xf1, 0x80, 0xcc}	/* cornish */

#define SRC_IP "198.252.55.135"				/* pencil */
#define DST_IP "198.252.55.133"				/* cornish */
#define DST_ETHER {0x0, 0xa0, 0xc9, 0xf1, 0x9, 0xcb}	/* cornish */

/*======================================================================*/

#include <oskit/dev/ethernet.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/io/netio.h>
#include <oskit/clientos.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <oskit/net/ether.h>
#include <oskit/net/socket.h>

#include <oskit/c/stdio.h>
#include <oskit/c/assert.h>

#include <oskit/fudp.h>

static unsigned char ethaddr[ETHER_ADDR_SIZE];
static oskit_netio_t *send_nio;

static oskit_error_t
net_receive(void *data, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	return 0;
}

/*
 * Set up the network and initialize `send_nio'.
 */
static void
setup_net(void)
{
	oskit_netio_t *recv_nio;
	oskit_etherdev_t **etherdevs;
	oskit_size_t ndev;
	oskit_error_t err;
	int i;

	/*
	 * Find all the Ethernet device nodes, BUT ONLY USE THE 1ST ONE.
	 */
	ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&etherdevs);
	assert(ndev);

	oskit_etherdev_getaddr(etherdevs[0], ethaddr);

	recv_nio = oskit_netio_create(net_receive, 0);
	assert(recv_nio);

	err = oskit_etherdev_open(etherdevs[0], 0, recv_nio, &send_nio);
	assert(! err);

	oskit_netio_release(recv_nio);
	for (i = 0; i < ndev; i++)
		oskit_etherdev_release(etherdevs[i]);
}

/*
 * Set up the FUDP socket stuff and get a socket into `out_sock'.
 */
static void
setup_socket(oskit_socket_t **out_sock)
{
	oskit_socket_factory_t *sfact;
	oskit_error_t err;
	struct sockaddr_in sin;
	unsigned char dsteth[] = DST_ETHER;
	struct in_addr ipaddr;
	oskit_socket_t *sock;

	/*
	 * Get a socket factory.
	 */
	err = fudp_init(&sfact);
	assert(! err);

	/*
	 * Create a socket.
	 */
	err = oskit_socket_factory_create(sfact, OSKIT_AF_INET,
					  OSKIT_SOCK_DGRAM, IPPROTO_UDP,
					  &sock);
	assert(! err);

	/*
	 * Bind the socket to a port with the ipaddr SRC_IP.
	 */
	sin.sin_len = htons(sizeof sin);
	sin.sin_family = OSKIT_AF_INET;
	sin.sin_addr.s_addr = inet_addr(SRC_IP);

	sin.sin_port = htons(SRC_PORT);
	err = oskit_socket_bind(sock, (struct oskit_sockaddr *)&sin,
				sizeof sin);
	assert(! err);

	/*
	 * Give the socket a netio.
	 */
	err = oskit_socket_setsockopt(sock, OSKIT_SOL_SOCKET, OSKIT_SO_NETIO,
				      send_nio, 0);
	assert(! err);

	/*
	 * Add ARP entries for the src and dst.
	 */
	ipaddr.s_addr = inet_addr(SRC_IP);
	err = farp_add(&ipaddr, ethaddr);
	assert(! err);
		
	ipaddr.s_addr = inet_addr(DST_IP);
	err = farp_add(&ipaddr, dsteth);
	assert(! err);

	/* Don't need this anymore. */
	oskit_socket_factory_release(sfact);

	*out_sock = sock;
}

/*
 * Send one byte on socket `sock'.
 */
static void
sendone(unsigned char c, oskit_socket_t *sock)
{
	struct sockaddr_in sin;
	oskit_size_t sent;
	oskit_error_t err;

	sin.sin_len = htons(sizeof sin);
	sin.sin_family = OSKIT_AF_INET;
	sin.sin_port = htons(DST_PORT);
	sin.sin_addr.s_addr = inet_addr(DST_IP);

	/* Send a datagram. */
	err = oskit_socket_sendto(sock, &c, 1, 0,
				  (struct oskit_sockaddr *)&sin, sizeof sin,
				  &sent);

	printf("sendone %c\n", c);
	if (sent != 1)
		printf("sendto: only sent %d\n", sent);
	if (err)
		printf("sendto: err 0x%08x\n", err);
}

int
main(int argc, char **argv)
{
	const char *p;
	oskit_socket_t *sock;

	oskit_clientos_init();
	start_net_devices();

	setup_net();
	setup_socket(&sock);

	p = msg;
	while (*p)
		sendone(*p++, sock);
	sendone('\0', sock);

	return 0;
}
