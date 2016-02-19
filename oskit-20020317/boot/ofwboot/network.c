/*
 * Copyright (c) 1997-2001 University of Utah and the Flux Group.
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <oskit/error.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/error.h>
#include <oskit/net/ether.h>
#include <oskit/net/bootp.h>
#include <oskit/net/socket.h>
#include <oskit/com/services.h>
#include <oskit/udplib.h>
#include <oskit/startup.h>
#include <oskit/boot/bootwhat.h>

#include "decls.h"

static oskit_etherdev_t	*dev;		/* Handle on our device */

/*
 * Our network params.
 */
char *ipaddr, *netmask, *gateway, *server;

/*
 * Initialize networkiness.
 *
 * Empty comments in column 1 indicate things that need to be undone upon
 * failure or in net_shutdown.
 *
 * Returns zero on success, an error code from <oskit/error.h> otherwise.
 */
oskit_error_t
net_init(void)
{
#define BUFSIZE 128
	struct bootp_net_info bpi;
	static char buf[BUFSIZE];
	oskit_etherdev_t **alldevs;
	int ndev;
	int i, ix;
	unsigned required_flags;
	oskit_socket_factory_t *fsc;
	oskit_error_t err;
	
        /*
         * Find all the Ethernet device nodes.
         */
 retry:
        ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&alldevs);
	if (ndev <= 0)
		panic("no Ethernet adaptors found!");

	dev = NULL;
	memset(&bpi, 0, sizeof bpi);

	/*
	 * Try bootp on each dev and use the first one that succeeds.
	 */
	ix = ndev;
	for (i = 0; i < ndev; i++) {
		if (dev) {
			/* Have choice, but must release the others anyway. */
			oskit_etherdev_release(alldevs[i]);
			continue;
		}
		if (bootp(alldevs[i], &bpi) != 0) {
			oskit_etherdev_release(alldevs[i]);
			continue;
		}

		required_flags = (BOOTP_NET_IP |
				  BOOTP_NET_NETMASK |
				  BOOTP_NET_GATEWAY |
				  BOOTP_NET_SERVER |
				  BOOTP_NET_HOSTNAME);
		if ((bpi.flags & required_flags) != required_flags) {
#define MISSING(flag, name) if ((bpi.flags & flag) == 0) \
			printf("bootp did not supply %s\n", name)
			MISSING(BOOTP_NET_IP, "my IP address");
			MISSING(BOOTP_NET_NETMASK, "my netmask");
			MISSING(BOOTP_NET_GATEWAY, "gateway address");
			MISSING(BOOTP_NET_SERVER, "server IP address");
			MISSING(BOOTP_NET_HOSTNAME, "my hostname");
#undef	MISSING
			oskit_etherdev_release(alldevs[i]);
			continue;
		}

/**/		dev = alldevs[i];
		ix = i;
	}

	if (dev == NULL) {
		printf("No bootp server found for any interface!");
		printf("  Try again? [n] ");
		fgets(buf, BUFSIZE, stdin);
		if (buf[0] == 'y' || buf[0] == 'Y')
			goto retry;
		return OSKIT_E_FAIL;
	}

	/*
	 * Now we know our bootp struct has all we need,
	 * so we copy the info out.
	 */
	ipaddr   = strdup(inet_ntoa(bpi.ip));
	netmask  = strdup(inet_ntoa(bpi.netmask));
	gateway  = strdup(inet_ntoa(bpi.gateway.addr[0]));
	server   = strdup(inet_ntoa(bpi.server));

#if 0
	bootp_dump(&bpi);
#endif

	/* Now init the fudp module. It will open the device */
	err = udplib_init(dev, bpi.hostname, ipaddr, netmask, gateway, &fsc);
	if (err)
		return err;

	/*
	 * Register to socket factory so that UDP might be useful for others.
	 * Its a long shot!
	 */
	oskit_register(&oskit_socket_factory_iid, (void *) fsc);
	  
	return 0;
}

void
net_shutdown()
{
	oskit_etherdev_release(dev);
}

/*
 * UDP bootinfo protocol
 */
#include <oskit/boot/bootwhat.h>

static oskit_socket_t *bisock;

void
bootinfo_init(void)
{
	static oskit_socket_factory_t *fsc;
	oskit_error_t err;
	struct oskit_sockaddr sockaddr;
	struct sockaddr_in *sin;

	if (fsc == 0) {
		/* note that lookup_first always returns null */
		oskit_lookup_first(&oskit_socket_factory_iid, (void *)&fsc);
		if (fsc == 0)
			goto fail;
	}

	/* Create the socket */
	err = oskit_socket_factory_create(fsc, OSKIT_AF_INET, OSKIT_SOCK_DGRAM,
					  IPPROTO_UDP, &bisock);
	if (err) {
		printf("bootinfo_init: oskit_socket_factory_create failed\n");
		goto fail;
	}

	/* Bind our side */
	sin = (struct sockaddr_in *)&sockaddr;
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_family = OSKIT_AF_INET;
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_port = htons(BOOTWHAT_SRCPORT);
	if (oskit_socket_bind(bisock, &sockaddr, sizeof(sockaddr))) {
		oskit_socket_release(bisock);
		printf("bootinfo_ack: oskit_socket_bind failed\n");
		goto fail;
	}
	return;

 fail:
	panic("tragic network initialization error");
}

int
bootinfo_request(boot_info_t *bootinfo)
{
	int j, err;
	oskit_size_t got;
	struct oskit_sockaddr sockaddr;
	struct sockaddr_in *sin;

	memset(bootinfo, 0, sizeof(*bootinfo));
#if 0
	bootinfo->status = BISTAT_SUCCESS;
	((boot_what_t *)bootinfo->data)->type = BIBOOTWHAT_TYPE_MB;
	strcpy(((boot_what_t *)bootinfo->data)->what.mb.filename,
	       "/export/dnard/linux/kernel");
	strcpy(((boot_what_t *)bootinfo->data)->cmdline,
	       "console=ttyS0");
	return 0;
#else
	bootinfo->opcode = BIOPCODE_BOOTWHAT_REQUEST;
	sin = (struct sockaddr_in *)&sockaddr;

	printf("Sending boot info request\n");

	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_family = OSKIT_AF_INET;
	sin->sin_addr.s_addr = inet_addr(BOOT_SERVER);
	sin->sin_port = htons(BOOTWHAT_DSTPORT);

	err = oskit_socket_sendto(bisock, bootinfo, sizeof(*bootinfo), 0,
				  &sockaddr, sizeof(sockaddr), &got);
	if (err)
		return err;
	if (got != sizeof(*bootinfo))
		return OSKIT_EMSGSIZE;

	/*
	 * Socket read is nonblocking, so wait for a few seconds
	 * if we get nothing and then try again.
	 */
	for (j = 0; j < 100; j++) {
		err = oskit_socket_recvfrom(bisock, bootinfo, sizeof(*bootinfo),
					    0, 0, 0, &got);
		/*
		 * Error (probably EWOULDBLOCK):
		 * wait awhile and try again.
		 */
		if (err) {
			if (err != OSKIT_EWOULDBLOCK)
				printf("Receive error %x\n", err);
			delay(100);
			continue;
		}

		/*
		 * Got a protocol packet, but not a reply:
		 * whine, wait and try again.
		 */
		if (bootinfo->opcode != BIOPCODE_BOOTWHAT_REPLY) {
			printf("Bad reply from server\n");
			delay(100);
			continue;
		}

		/*
		 * Got good stuff: return!
		 */
		return 0;
	}

	return -1;
#endif
}

void
bootinfo_ack(void)
{
	int i;
	boot_info_t bootinfo;
	oskit_size_t got;
	struct oskit_sockaddr sockaddr;
	struct sockaddr_in *sin;
	
	bootinfo.opcode = BIOPCODE_BOOTWHAT_ACK;
	bootinfo.status = BISTAT_SUCCESS;

	sin = (struct sockaddr_in *)&sockaddr;
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_family = OSKIT_AF_INET;
	sin->sin_addr.s_addr = inet_addr(BOOT_SERVER);
	sin->sin_port = htons(BOOTWHAT_DSTPORT);

	for (i = 0; i < 3; i++)
		oskit_socket_sendto(bisock, &bootinfo, sizeof(bootinfo), 0,
				    &sockaddr, sizeof(sockaddr), &got);
}

