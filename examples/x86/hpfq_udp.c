/*
 * Copyright (c) 1998-2000 The University of Utah and the Flux Group.
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
 * An example program demonstrating the H-PFQ library.
 * It sets up the hierarchial share tree with three leaves and then
 * sends `udpbufsize' bytes to each in a loop for `SECONDS' seconds.
 * This example is only useful if hpfq_udp_tally is running on the remote
 * machine specified by DST_PORT/DST_IP/DST_ETHER
 * Note that the liboskit_linux_dev library must have been built with
 * the `HPFQ' CPP symbol defined.
 * This is most easily achieved by configuring the oskit with `--enable-hpfq'.
 */

/*
 * Configure appropriately.
 * These values must agree with hpfq_udp_tally.c, which see.
 */
#define PORTBASE 10000
#define DST_PORT 5050
#if 0
# define SRC_IP "155.99.214.161"		/* shaky */
# define DST_IP "155.99.214.162"		/* risky */
# define DST_ETHER {0x0, 0x0, 0xc0, 0x26, 0xc, 0x9f} /* risky */
#else
# define SRC_IP "155.99.212.51"				/* pencil */
# define DST_IP "155.99.212.96"				/* cornish */
# define DST_ETHER {0x0, 0xa0, 0xc9, 0xf1, 0x80, 0xcc}	/* cornish */
#endif
#define SECONDS 10			/* how long to run for */

#include <oskit/net/ether.h>
#include <oskit/c/netinet/ip.h>
#include <oskit/c/netinet/udp.h>

#define HDRSIZE (ETHER_HDR_SIZE + sizeof(struct ip) + sizeof(struct udphdr))
/* 2000bits is supposed to be avg packet size. */
const int udpbufsize = ETH_MAX_PACKET - HDRSIZE;	/* max */
#if 0
const int udpbufsize = ETH_MIN_PACKET - HDRSIZE;	/* min */
const int udpbufsize = 2000/8 - 4 - HDRSIZE;		/* avg, -4 for CRC */
#endif
static char udpbuf[ETH_MAX_PACKET - HDRSIZE];

/*======================================================================*/

#include <oskit/dev/dev.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/linux.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <oskit/net/socket.h>

#include <oskit/io/netio.h>
#include <oskit/io/bufio.h>

#include <oskit/c/stdio.h>
#include <oskit/c/assert.h>

#include <oskit/hpfq.h>
#include <oskit/fudp.h>

/* these things should be defined elsewhere, I think */
#define TIMER_FREQ 100
extern void osenv_timer_init();

static oskit_etherdev_t *edev;
static oskit_netio_t *recv_nio, *send_nio;
static oskit_netio_t *nio1, *nio2, *nio3;
static oskit_socket_t *sock1, *sock2, *sock3;

static unsigned char ethaddr[ETHER_ADDR_SIZE];

static void
timer()
{
	static int count = 0, sec=0;
	if (++count >= 100) {
		count = 0;
		if (++sec > SECONDS) {
			pfq_dump_stats();
			pfq_sched_release(oskit_pfq_root);
			oskit_etherdev_release(edev);
			oskit_netio_release(send_nio);
			oskit_netio_release(recv_nio);
			exit(0);
		}
		printf("%d second(s) - %d\n", sec, udpbufsize);
	}
}

static oskit_error_t
net_receive(void *data, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	return 0;
}

static void
sendit(oskit_socket_t *sock)
{
	struct sockaddr_in sin;
	oskit_size_t sent;
	oskit_error_t err;

	sin.sin_len = htons(sizeof sin);
	sin.sin_family = OSKIT_AF_INET;
	sin.sin_port = htons(DST_PORT);
	sin.sin_addr.s_addr = inet_addr(DST_IP);

	/* Send a datagram. */
	err = oskit_socket_sendto(sock, udpbuf, udpbufsize, 0,
				  (struct oskit_sockaddr *)&sin, sizeof sin,
				  &sent);
	if (sent != udpbufsize)
		printf("sendto: only sent %d\n", sent);
	if (err)
		printf("sendto: err 0x%08x\n", err);
}

/*
 * Initalizes network stuff.
 * Sets up the `send_nio' global.
 */
static void
setup_net(void)
{
	oskit_etherdev_t **etherdevs;
	oskit_size_t ndev;
	oskit_error_t err;

	/*
	 * Find all the Ethernet device nodes, but only use the 1st one.
	 */
	ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&etherdevs);
	assert(ndev);

	edev = etherdevs[0];
	oskit_etherdev_getaddr(etherdevs[0], ethaddr);

	recv_nio = oskit_netio_create(net_receive, 0);
	assert(recv_nio);

	err = oskit_etherdev_open(etherdevs[0], 0, recv_nio, &send_nio);
	assert(! err);
}

/*
 * Sets up the H-PFQ tree.
 * Initalizes nio1, nio2, ...
 *
 * Makes a tree like:
 *
 *			 (root, ssf)
 *			   /    \
 *			  /      \
 *		(node, sff, 50%)  (leaf3, 50%)
 *		   /      \
 *		  /        \
 *	   (leaf1, 75%)  (leaf2, 25%)
 *
 * nioN corresponds to leafN.
 */
static void
setup_pfq(void)
{
	pfq_sched_t *root;
	pfq_sched_t *node;
	pfq_leaf_t *leaf1, *leaf2, *leaf3;
	oskit_error_t err;

	err = pfq_ssf_create_root(send_nio, &root);
	assert(! err);

	err = pfq_sff_create(&node); assert(! err);

	err = pfq_leaf_create(&leaf1); assert(! err);
	err = pfq_leaf_create(&leaf2); assert(! err);
	err = pfq_leaf_create(&leaf3); assert(! err);

	err = pfq_sched_add_child(root, node, 0.50);
	assert(! err);
	err = pfq_sched_add_child(node, (pfq_sched_t *) leaf1, 0.75);
	assert(! err);
	err = pfq_sched_add_child(node, (pfq_sched_t *) leaf2, 0.25);
	assert(! err);
	err = pfq_sched_add_child(root, (pfq_sched_t *) leaf3, 0.50);
	assert(! err);

	err = pfq_leaf_get_netio(leaf1, &nio1); assert(! err);
	err = pfq_leaf_get_netio(leaf2, &nio2); assert(! err);
	err = pfq_leaf_get_netio(leaf3, &nio3); assert(! err);

	oskit_pfq_root = root;
	oskit_pfq_reset_path = pfq_reset_path;
}

static void
setup_sockets(void)
{
	oskit_socket_factory_t *sfact;
	oskit_error_t err;
	struct sockaddr_in sin;
	unsigned char dsteth[] = DST_ETHER;
	struct in_addr ipaddr;

	/*
	 * Get a socket factory.
	 */
	err = fudp_init(&sfact);
	assert(! err);

	/*
	 * Create a few sockets.
	 */
	err = oskit_socket_factory_create(sfact, OSKIT_AF_INET,
					  OSKIT_SOCK_DGRAM, IPPROTO_UDP,
					  &sock1);
	assert(! err);
	err = oskit_socket_factory_create(sfact, OSKIT_AF_INET,
					  OSKIT_SOCK_DGRAM, IPPROTO_UDP,
					  &sock2);
	assert(! err);
	err = oskit_socket_factory_create(sfact, OSKIT_AF_INET,
					  OSKIT_SOCK_DGRAM, IPPROTO_UDP,
					  &sock3);
	assert(! err);

	/*
	 * Bind the sockets to ports with the ipaddr SRC_IP.
	 */
	sin.sin_len = htons(sizeof sin);
	sin.sin_family = OSKIT_AF_INET;
	sin.sin_addr.s_addr = inet_addr(SRC_IP);

	sin.sin_port = htons(PORTBASE + 1);
	err = oskit_socket_bind(sock1, (struct oskit_sockaddr *)&sin,
				sizeof sin);
	assert(! err);

	sin.sin_port = htons(PORTBASE + 2);
	err = oskit_socket_bind(sock2, (struct oskit_sockaddr *)&sin,
				sizeof sin);
	assert(! err);

	sin.sin_port = htons(PORTBASE + 3);
	err = oskit_socket_bind(sock3, (struct oskit_sockaddr *)&sin,
				sizeof sin);
	assert(! err);

	/*
	 * Give the sockets netios.
	 */
	err = oskit_socket_setsockopt(sock1, OSKIT_SOL_SOCKET, OSKIT_SO_NETIO,
				      nio1, 0);
	assert(! err);
	err = oskit_socket_setsockopt(sock2, OSKIT_SOL_SOCKET, OSKIT_SO_NETIO,
				      nio2, 0);
	assert(! err);
	err = oskit_socket_setsockopt(sock3, OSKIT_SOL_SOCKET, OSKIT_SO_NETIO,
				      nio3, 0);
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
}

int
main()
{
	oskit_clientos_init();
	start_net_devices();

	setup_net();
	setup_pfq();
	setup_sockets();

	osenv_timer_init();
	osenv_timer_register(timer, TIMER_FREQ);

	for (;;) {
		sendit(sock1);
		sendit(sock2);
		sendit(sock3);
	}
}
