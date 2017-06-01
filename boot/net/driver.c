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
#include <malloc.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <oskit/com/queue.h>

#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/net.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/error.h>
#include <oskit/dev/osenv_device.h>
#include <oskit/udplib.h>
#include <oskit/startup.h>

#include <oskit/net/bootp.h>

#include "timer.h"
#include "reboot.h"

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

struct ourinfo {
	int	ethif;
	struct bootp_net_info bpi;
	struct in_addr gateway;
	struct in_addr dns_server;
	char hostname[64];
	char domainname[64];
} ourinfo = { -1, { 0 }, { 0 }, { 0 } };

static oskit_etherdev_t	*dev;		/* Handle on our device */
#ifdef FREEBSD_NET
static struct oskit_freebsd_net_ether_if *eif;
#endif
#define getline(buf, size) fgets(buf, size, stdin);

/*
 * Probe and initialize all the network devices
 *
 * Overrides the OSKit version so we can be more selective about including
 * network device drivers.
 */
void
start_net_devices(void)
{
	oskit_osenv_t *osenv = start_osenv();

	oskit_dev_init(osenv);
	oskit_linux_init_osenv(osenv);
#ifdef UTAHTESTBED
	oskit_linux_init_ethernet_eepro100();
#else
	oskit_linux_init_net();
#endif
	oskit_dev_probe();
}

/*
 * Initialize networkiness.
 *
 * Empty comments in column 1 indicate things that need to be undone upon
 * failure or in net_shutdown.
 *
 * Returns zero on success, an error code from <oskit/error.h> otherwise.
 */
oskit_error_t
net_init(oskit_osenv_t *osenv)
{
#define BUFSIZE 128
	struct bootp_net_info bpi;
	struct in_addr gw;
	char hn[64];
	static char buf[BUFSIZE];
	oskit_etherdev_t **alldevs;
	int ndev;
	int i, ix;
	unsigned required_flags;
	int interactive = 0;
	oskit_socket_factory_t *fsc;
#ifdef  FREEBSD_NET
	oskit_error_t err;
	const char *eif_name;
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

	/*
	 * If stored host info is valid, use it
	 */
	if (ourinfo.ethif >= 0 && ourinfo.ethif < ndev) {
		ix = ourinfo.ethif;
		dev = alldevs[ix];
		bpi = ourinfo.bpi;
		goto gotsavedinfo;
	}

	dev = NULL;
	memset(&bpi, 0, sizeof bpi);

	/*
	 * Prompt user for info
	 */
	if (interactive) {
		char *cp;

		do {
			printf("Enter ethernet IF to use [%d-%d]: ", 0, ndev-1);
			getline(buf, BUFSIZE);
			ix = atoi(buf);
			if (ix < 0)
				goto gotinfo;
		} while (ix >= ndev);
		do {
			printf("Enter IP address: ");
			getline(buf, BUFSIZE);
		} while (inet_aton(buf, &bpi.ip) == 0);
		do {
			printf("Enter netmask: ");
			getline(buf, BUFSIZE);
		} while (inet_aton(buf, &bpi.netmask) == 0);
		do {
			printf("Enter gateway IP address: ");
			getline(buf, BUFSIZE);
		} while (inet_aton(buf, &gw) == 0);
		bpi.gateway.addr = &gw;
		bpi.gateway.len = 1;
		printf("Enter hostname: ");
		getline(buf, BUFSIZE);
		if ((cp = strchr(buf, '\n')) != 0)
			*cp = '\0';
		memcpy(hn, buf, sizeof hn);
		bpi.hostname = hn;
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
				  BOOTP_NET_HOSTNAME);
		if ((bpi.flags & required_flags) != required_flags) {
#define MISSING(flag, name) if ((bpi.flags & flag) == 0) \
			printf("bootp did not supply %s\n", name)
			MISSING(BOOTP_NET_IP, "my IP address");
			MISSING(BOOTP_NET_NETMASK, "my netmask");
			MISSING(BOOTP_NET_GATEWAY, "gateway address");
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
		getline(buf, BUFSIZE);
		if (buf[0] == 'y' || buf[0] == 'Y')
			goto retry;
		printf("Enter bootp information manually? [y] ");
		getline(buf, BUFSIZE);
		if (buf[0] != 'n' && buf[0] != 'N') {
			interactive = 1;
			goto retry;
		}
		return OSKIT_E_FAIL;
	}

	/*
	 * All is well, stash the bootp info for next time
	 */
	assert(ix >= 0 && ix < ndev);
	ourinfo.ethif = ix;
	ourinfo.bpi = bpi;
	ourinfo.gateway = bpi.gateway.addr[0];
	ourinfo.bpi.gateway.addr = &ourinfo.gateway;
	ourinfo.bpi.gateway.len = 1;
	ourinfo.dns_server = bpi.dns_server.addr[0];
	ourinfo.bpi.dns_server.addr = &ourinfo.dns_server;
	ourinfo.bpi.dns_server.len = 1;
	strncpy(ourinfo.hostname, bpi.hostname, sizeof ourinfo.hostname);
	ourinfo.bpi.hostname = ourinfo.hostname;
	strncpy(ourinfo.domainname, bpi.domainname, sizeof ourinfo.domainname);
	ourinfo.bpi.domainname = ourinfo.domainname;
	patch_clone(&ourinfo, sizeof ourinfo);

 gotsavedinfo:
	/*
	 * Now we know our bootp struct has all we need,
	 * so we copy the info out.
	 */
	ipaddr   = strdup(inet_ntoa(bpi.ip));
	netmask  = strdup(inet_ntoa(bpi.netmask));
	gateway  = strdup(inet_ntoa(bpi.gateway.addr[0]));

	if (bpi.flags & BOOTP_NET_DOMAINNAME)
		domain = strdup(bpi.domainname);
	
	if (bpi.flags & BOOTP_NET_DNS_SERVER) {
		struct bootp_addr_array ns;
		int x;

		ns = bpi.dns_server;
		nameservers = mustcalloc(1, (sizeof(char*) * ns.len) + 1);
		for (x = 0; x < ns.len; x++)
			nameservers[x] = strdup(inet_ntoa(ns.addr[x]));
		nameservers[ns.len] = NULL;
	}

	/* Hostnamelen needs to be word aligned. */
	hostname = (char *)mustcalloc(strlen(bpi.hostname) + 3, 1);
	strcpy(hostname, bpi.hostname);
	hostnamelen = (strlen(hostname) + 3) & ~3;

#ifdef  FREEBSD_NET
	err = start_conf_network_init(start_osenv(), &fsc);
	assert(!err);

	err = start_conf_network_open_eif(dev, &eif);
	assert(!err); /* XXX */

	eif_name = start_conf_network_eifname(ix);

	err = start_conf_network_eifconfig(eif, eif_name, ipaddr, netmask);
	assert(!err);
#else
	/* Now init the fudp module. It will open the device */
	udplib_init(dev, hostname, ipaddr, netmask, gateway, &fsc);
#endif
	/*
	 * This writes out the DNS files. We need the freebsd stub
	 * function below since it thinks its dealing with the freebsd
	 * network stack.
	 */
	start_conf_network_host(hostname, gateway, domain, nameservers);
	
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
#ifdef  FREEBSD_NET
	start_conf_network_close_eif(eif);
#endif
	oskit_etherdev_release(dev);
}

#ifndef	FREEBSD_NET
/*
 * Stub for start_conf_network_host
 */
int
oskit_freebsd_net_add_default_route(char *ignored)
{
	return 0;
}
#endif
