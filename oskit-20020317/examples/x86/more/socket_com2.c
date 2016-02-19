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
 * This is the test program for the COM interfaces for the TCP/IP stack
 * It first does two fingers and then waits for incoming date requests.
 *
 * It assumes that libc's sockaddr_in is compatible with the
 * COM interface's oskit_sockaddr type.
 *
 * Note the difference to socket_com.c about the way the networking stack is
 * initialized. In socket_com.c, the act of opening the ethernet device (and
 * thus exchanging netio instances) is done by the networking code in
 * oskit_freebsd_net_open_ether_if. This does not allow us to determine
 * which netio should be used to send by the networking stack.
 * On the other hand, if we want to perform the exchange of netios ourselves,
 * we must be provided with the actual netio the networking stack uses for
 * receive. This is done by oskit_freebsd_net_prepare_ether_if.
 *
 * If that difference doesn't make sense to you, think of situations where
 * you would want to use the networking stack with netio objects
 * that do not necessarily result from opening an ethernet device.
 * One such instance could be the insertion of a netio compatible fanout
 * device on the send path, which can be used to implement a tcpdump-like
 * facility. See start_network_with_dump.c in the unsupported directory
 * for an example.
 */
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/error.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/freebsd.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/timer.h>
#include <oskit/com/listener.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/net/freebsd.h>
#include <oskit/net/socket.h>
#include <oskit/io/netio_fanout.h>

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
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
 * this is a simple finger client, programmed to the socket COM interface
 */
void
fingerclient(char *name, char *host)
{
	oskit_socket_t 		*so;
	oskit_stream_t 		*oso;
	oskit_error_t 		err;
	int			namelen;
	oskit_u32_t 		retval, msg_len;

	struct  sockaddr_in 	addr;
	char			message[256];

	if ((err = oskit_socket_factory_create(socket_create, OSKIT_AF_INET,
			OSKIT_SOCK_STREAM, IPPROTO_TCP, &so))) {
		printf("socket_create: errno = %x\n", (int)err);
		return;
	}

	/* fill in the sin_addr structure */
	memset(&addr, 0, namelen = sizeof(addr));
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_family = OSKIT_AF_INET;
	addr.sin_port = htons(79);

	if ((err = oskit_socket_connect(so,
		(struct oskit_sockaddr *)&addr, sizeof(addr))))
	{
		printf("connect: errno = 0x%x: %s\n", (int)err, strerror(err));
		return;
	}

	if ((err = oskit_socket_query(so, &oskit_stream_iid, (void **)&oso))) {
		printf("query: errno is 0x%x\n", (int)err);
		return;
	}

	sprintf(message, "%s\r\n", name);
	err = oskit_stream_write(oso,
		message, msg_len = strlen(message), &retval);
	if (err) {
		printf("oskit_stream_write returned = %x\n", (int)err);
		return;
	}

	do {
		err = oskit_stream_read(oso, message,
			sizeof(message), &retval);
		if (!err) {
			int i;
			for (i = 0; i < retval; i++)
				putchar(message[i]);
		} else {
			printf("read: errno %x\n", (int)err);
			return;
		}
	} while (retval > 0);

	oskit_stream_release(oso);
	oskit_socket_release(so);
}

/* timeout handler - just raise a flag */
static int
timeout(struct oskit_iunknown *clock, void *arg)
{
	return *(int *)arg = 1;
}

/*
 * this is a simple server responding to requests on port 13 (daytime)
 */
void
dateserver(int seconds)
{
	oskit_socket_t	*so, *nso;
	oskit_stream_t	*oso;
	int 	err, namelen;
	struct  sockaddr_in addr;
	oskit_timer_t	*timer;
	oskit_listener_t *l;
	oskit_itimerspec_t expire;
	oskit_timespec_t  tod;
	oskit_clock_t	*clock = oskit_clock_init();
	volatile int stop = 0;

	if ((err = oskit_socket_factory_create(socket_create,
		OSKIT_AF_INET, OSKIT_SOCK_STREAM, IPPROTO_TCP, &so))) {
		printf("oskit_freebsd_net_socket_create: errno = %d\n", err);
		return;
	}

	/* fill in the sin_addr structure */
	memset(&addr, 0, namelen = sizeof(addr));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = OSKIT_AF_INET;
	addr.sin_port = htons(13);

	if ((err = oskit_socket_bind(so,
			(struct oskit_sockaddr *)&addr, sizeof(addr)))) {
		printf("bind: errno = %d\n", err);
		return;
	}

	if ((err = oskit_socket_listen(so, 5))) {
		printf("listen: errno = %d\n", err);
		return;
	}

	oskit_rtc_get(&tod);
	LOCAL_TO_GMT(&tod);
	oskit_clock_settime(clock, &tod);
	oskit_clock_createtimer(clock, &timer);
	oskit_timer_setlistener(timer,
		l = oskit_create_listener(timeout, (void *)&stop));
	oskit_listener_release(l);

	memset(&expire, 0, sizeof expire);
	expire.it_value.tv_sec = seconds;
	oskit_timer_settime(timer, 0, &expire);

	for (;!stop;) {
		char time_message[BUFSIZ];
		oskit_u32_t  msg_len, written;
		oskit_timespec_t	currenttime;

		if ((err = oskit_socket_accept(so,
			(struct oskit_sockaddr *)&addr, &namelen, &nso)))
		{
			printf("accept: errno = %d\n", err);
			return;
		}

		if ((err = oskit_socket_query(nso, &oskit_stream_iid,
			(void **)&oso))) {
			printf("query: errno = %d\n", err);
			return;
		}

		oskit_clock_gettime(clock, &currenttime);
		sprintf(time_message,
			"At the tone, the time will be %u:%u ... ding\n",
			currenttime.tv_sec, currenttime.tv_nsec);

		msg_len = strlen(time_message);

		err = oskit_stream_write(oso,
			time_message, msg_len, &written);
		if (err)
		{
			printf("oskit_stream_write returned = %d\n", err);
			return;
		} else {
			printf("write %d out of %d bytes\n",
				written, msg_len);
		}

		oskit_stream_release(oso);
		oskit_socket_release(nso);
	}
	printf("bailing out of loop...\n");
	oskit_socket_release(so);
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

	oskit_clientos_init();
	start_net_devices();

	/* initialize network code */
	err = oskit_freebsd_net_init(start_osenv(), &socket_create);
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
	 * prepare a net_ether_info struct - this gives us the recv_nio
	 * to be passed to the driver
	 */
	err = oskit_freebsd_net_prepare_ether_if(&eif);
	assert(!err);

	/* Open this adaptor, exchanging net_io devices - put the netio
	 * device for outgoing packets in net_ether_info struct
	 */
	err = oskit_etherdev_open(dev, 0, eif->recv_nio, &eif->send_nio);
	if (err) {
		osenv_log(OSENV_LOG_ERR, "Error opening ethercard\n");
		return err;
	}

	/* initialize hardware address */
        oskit_etherdev_getaddr(dev, eif->haddr);

	/* configure the interface */
	err = oskit_freebsd_net_ifconfig(eif, IFNAME, IPADDR, NETMASK);
	assert(!err);

	err = oskit_freebsd_net_add_default_route(GATEWAY);
	if (err)
		printf("couldn't add default route (%d)\n", err);

	fingerclient("daemon", "155.99.212.1");		/* fast.cs.utah.edu */
	fingerclient("oskit", "155.99.212.1");		/* fast */
	printf("now responding to date (port 13) requests for 30 seconds\n");
	printf("type 'telnet %s 13' to test\n", IPADDR);
	dateserver(30);

	/* release net_io devices */
	oskit_freebsd_net_close_ether_if(eif);
	exit(0);
}
