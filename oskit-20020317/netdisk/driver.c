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

#include "driver.h"
#include "timer.h"

static oskit_etherdev_t	*dev;		/* Handle on our device */
#ifdef FREEBSD_NET
static struct oskit_freebsd_net_ether_if *eif;
#endif

/*
 * Our network params.
 */
char			*hostname;
int			hostnamelen;	/* word aligned length of hostname */
char			*ipaddr;
char			*netmask;
char			*gateway;
char			**nameservers;
char			*domain;


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
	struct in_addr gw, ns;
	char hn[64], dn[64];
	static char buf[BUFSIZE];
	oskit_etherdev_t **alldevs;
	int ndev;
	int i, ix;
	unsigned required_flags;
	int interactive = 0;
	oskit_socket_factory_t *fsc;
#ifdef  FREEBSD_NET
	oskit_error_t err;
#endif
	
        /*
         * Find all the Ethernet device nodes.
         */
 retry:
        ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&alldevs);

#ifdef BOOTP_IF
	if (ndev < BOOTP_IF)
		panic("BOOTP Ethernet adaptor %d not found!", BOOTP_IF);
#else
	if (ndev <= 0)
		panic("no Ethernet adaptors found!");
#endif
	dev = NULL;
	memset(&bpi, 0, sizeof bpi);

	/*
	 * Prompt user for info
	 */
	if (interactive) {
		char *cp;

		do {
			printf("Enter ethernet IF to use [%d-%d]: ", 0, ndev-1);
			fgets(buf, BUFSIZE, stdin);
			ix = atoi(buf);
			if (ix < 0)
				goto gotinfo;
		} while (ix >= ndev);
		do {
			printf("Enter IP address: ");
			fgets(buf, BUFSIZE, stdin);
		} while (inet_aton(buf, &bpi.ip) == 0);
		do {
			printf("Enter netmask: ");
			fgets(buf, BUFSIZE, stdin);
		} while (inet_aton(buf, &bpi.netmask) == 0);
		do {
			printf("Enter gateway IP address: ");
			fgets(buf, BUFSIZE, stdin);
		} while (inet_aton(buf, &gw) == 0);
		bpi.gateway.addr = &gw;
		bpi.gateway.len = 1;
		printf("Enter hostname: ");
		fgets(buf, BUFSIZE, stdin);
		if ((cp = strchr(buf, '\n')) != 0)
			*cp = '\0';
		memcpy(hn, buf, sizeof hn);
		bpi.hostname = hn;
		printf("Enter domainname: ");
		fgets(buf, BUFSIZE, stdin);
		if ((cp = strchr(buf, '\n')) != 0)
			*cp = '\0';
		memcpy(dn, buf, sizeof dn);
		bpi.domainname = dn;
		do {
			printf("Enter nameserver IP address: ");
			fgets(buf, BUFSIZE, stdin);
		} while (inet_aton(buf, &ns) == 0);
		bpi.dns_server.addr = &ns;
		bpi.dns_server.len = 1;
		dev = alldevs[ix];
		goto gotinfo;
	}

	/*
	 * Try bootp on each dev and use the first one that succeeds.
	 */
	ix = ndev;
	for (i = 0; i < ndev; i++) {
#ifdef BOOTP_IF
		if (i != BOOTP_IF) {
			oskit_etherdev_release(alldevs[i]);
			continue;
		}
#else
		if (dev) {
			/* Have choice, but must release the others anyway. */
			oskit_etherdev_release(alldevs[i]);
			continue;
		}
#endif
		if (bootp(alldevs[i], &bpi) != 0) {
			oskit_etherdev_release(alldevs[i]);
			continue;
		}

		required_flags = (BOOTP_NET_IP |
				  BOOTP_NET_NETMASK |
				  BOOTP_NET_GATEWAY |
				  BOOTP_NET_DNS_SERVER |
				  BOOTP_NET_DOMAINNAME |
				  BOOTP_NET_HOSTNAME);
		if ((bpi.flags & required_flags) != required_flags) {
#define MISSING(flag, name) if ((bpi.flags & flag) == 0) \
			printf("bootp did not supply %s\n", name)
			MISSING(BOOTP_NET_IP, "my IP address");
			MISSING(BOOTP_NET_NETMASK, "my netmask");
			MISSING(BOOTP_NET_GATEWAY, "gateway address");
			MISSING(BOOTP_NET_DNS_SERVER, "DNS servers");
			MISSING(BOOTP_NET_DOMAINNAME, "domainname");
			MISSING(BOOTP_NET_HOSTNAME, "my hostname");
#undef	MISSING
			oskit_etherdev_release(alldevs[i]);
			continue;
		}

/**/		dev = alldevs[i];
		ix = i;
	}
 gotinfo:
	if (dev == NULL) {
#ifdef UTAHTESTBED
		/*
		 * XXX fer now hack
		 */
		static int tried;
		if (tried++ < 5)
			goto retry;
#endif
#ifdef BOOTP_IF
		printf("No bootp server found for eth%d!", BOOTP_IF);
#else
		printf("No bootp server found for any interface!");
#endif
		printf("  Try again? [n] ");
		fgets(buf, BUFSIZE, stdin);
		if (buf[0] == 'y' || buf[0] == 'Y')
			goto retry;
		printf("Enter bootp information manually? [y] ");
		fgets(buf, BUFSIZE, stdin);
		if (buf[0] != 'n' && buf[0] != 'N') {
			interactive = 1;
			goto retry;
		}
		return OSKIT_E_FAIL;
	}

	/*
	 * Now we know our bootp struct has all we need,
	 * so we copy the info out.
	 */
	ipaddr   = strdup(inet_ntoa(bpi.ip));
	netmask  = strdup(inet_ntoa(bpi.netmask));
	gateway  = strdup(inet_ntoa(bpi.gateway.addr[0]));
	domain   = strdup(bpi.domainname);

	{
		struct bootp_addr_array ns;
		int x;

		ns = bpi.dns_server;
		nameservers = mustcalloc(1, (sizeof(char*) * ns.len) + 1);
		for (x = 0; x < ns.len; x++)
			nameservers[x] = strdup(inet_ntoa(ns.addr[x]));
		nameservers[ns.len] = NULL;
	}

	/* Hostnamelen needs to be word aligned for RPC code. */
	hostname = (char *)mustcalloc(strlen(bpi.hostname) + 3, 1);
	strcpy(hostname, bpi.hostname);
	hostnamelen = (strlen(hostname) + 3) & ~3;

#ifdef  DEBUG
	bootp_dump(&bpi);
#endif
	printf("I am %s.%s (IP: %s, GW: %s, mask: %s)\n",
	       hostname, domain, ipaddr, gateway, netmask);

#ifdef  FREEBSD_NET
	err = start_conf_network_init(start_osenv(), &fsc);
	assert(!err);

	err = start_conf_network_eifstart(i, ipaddr, netmask);
	assert(!err);

	start_conf_network_host(hostname, gateway, domain, nameservers);
#else
	/* Now init the fudp module. It will open the device */
	udplib_init(dev, hostname, ipaddr, netmask, gateway, &fsc);

	
	/*
	 * This writes out the DNS files. We need the freebsd stub
	 * function below since it thinks its dealing with the freebsd
	 * network stack.
	 */
	start_conf_network_host(hostname, gateway, domain, nameservers);
#endif
	
	/*
	 * Register to socket factory so that UDP might be useful for others.
	 * Its a long shot!
	 */
#ifdef  PTHREADS
	pthread_init_socketfactory(fsc);
#else
	oskit_register(&oskit_socket_factory_iid, (void *) fsc);
#endif
	  
	return 0;
}

void
net_shutdown()
{
#ifdef  FREEBSD_NET
	start_conf_network_close_eif(eif);
#endif
	oskit_etherdev_release(dev);
}

/*
 * Stub for start_conf_network_host
 */
int
oskit_freebsd_net_add_default_route(char *ignored)
{
	return 0;
}
