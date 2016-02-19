/*
 * Copyright (c) 1998-2002 The University of Utah and the Flux Group.
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
#include <stdlib.h>
#include <string.h>

#include <oskit/error.h>
#include <oskit/startup.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/dev.h>
#include <oskit/net/bootp.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <oskit/tmcp.h>
#include "tmcp.h"

struct myhost {
	char *name;
	char *domain;
	char *ipaddr;
	char *netmask;
	char *gateway;
	char *nameserver;
};

static struct myhost myhostinfo;
static int ifnum = -1;

/*
 * atexit handler
 */
static void
tmcp_exit(void *arg)
{
	tmcp_shutdown((int)arg, 1);
}

/*
 * Contact the emulab.net Testbed Master Control Protocol (TMCP) server to
 * obtain all the essential interface and host information for the current
 * experiment.
 */
int
start_tmcp(int waitforall)
{
	int err, ndev, i;
	oskit_etherdev_t **edev;
	struct bootp_net_info bpi;
	int tries = 1;

	start_net_devices();

        ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&edev);
	if (ndev <= 0)
		return OSKIT_EINVAL;

 retry:
	/*
	 * Bootp to find out our primary identity.
	 */
	err = OSKIT_EINVAL;
	for (i = 0; i < ndev; i++) {
		bpi.flags = 0;
		err = bootp_gen(edev[i], &bpi, tries, BOOTP_TIMEOUT);
		if (err == 0 && (bpi.flags & BOOTP_NET_IP) != 0) {
			struct in_addr nextip, dstip;

			inet_aton(TMCP_HOST, &dstip);

			/*
			 * If server is on a different subnet,
			 * make sure there is a gateway and use that
			 * as the next hop.
			 */
			if ((bpi.flags & BOOTP_NET_NETMASK) != 0 &&
			    (bpi.ip.s_addr & bpi.netmask.s_addr) !=
			    (dstip.s_addr & bpi.netmask.s_addr) &&
			    (bpi.flags & BOOTP_NET_GATEWAY) != 0)
				nextip = bpi.gateway.addr[0];
			else
				nextip = bpi.ip;

			err = tmcp_getinfo(edev[i], i, &bpi.ip, &nextip,
					   &dstip, TMCP_PORT, waitforall);
			break;
		}
	}

	if (err == OSKIT_ETIMEDOUT) {
		tries *= 2;
		if (tries < BOOTP_MAX_RETRIES)
			goto retry;
	}

	/*
	 * Record BOOTP obtained host info
	 * and map TMCP notion of interface number to OSKit notion.
	 */
	if (err == 0) {
		ifnum = i;

		if (bpi.flags & BOOTP_NET_IP)
			myhostinfo.ipaddr = strdup(inet_ntoa(bpi.ip));
		if (bpi.flags & BOOTP_NET_NETMASK)
			myhostinfo.netmask = strdup(inet_ntoa(bpi.netmask));
		if (bpi.flags & BOOTP_NET_GATEWAY)
			myhostinfo.gateway =
				strdup(inet_ntoa(bpi.gateway.addr[0]));
		if (bpi.flags & BOOTP_NET_HOSTNAME)
			myhostinfo.name = strdup(bpi.hostname);
		if (bpi.flags & BOOTP_NET_DOMAINNAME)
			myhostinfo.domain = strdup(bpi.domainname);
		if (bpi.flags & BOOTP_NET_DNS_SERVER)
			myhostinfo.nameserver =
				strdup(inet_ntoa(bpi.dns_server.addr[0]));

		/*
		 * XXX map TB provided interface nums to OSKit interface nums
		 */
		tmcp_mapifs(edev, ndev);
	}

	for (i = 0; i < ndev; i++)
		oskit_etherdev_release(edev[i]);
	free(edev);

#if 0	/* XXX causes crash right now */
	if (err == 0)
		startup_atexit(tmcp_exit, 0);
#endif

	return err;
}

int
start_tmcp_getinfo_if(int ifn, char **addrp, char **maskp)
{
	struct in_addr addr, mask;
	int err;

	err = tmcp_if2addr(ifn, &addr, &mask);
	if (err == 0) {
		if (addrp != 0)
			*addrp = strdup(inet_ntoa(addr));
		if (maskp != 0)
			*maskp = strdup(inet_ntoa(mask));

		return 0;
	}

	/*
	 * XXX if they are configuring the control interface,
	 * return our BOOTP host info.
	 */
	if (ifn == ifnum) {
		if (addrp != 0)
			*addrp = strdup(myhostinfo.ipaddr);
		if (maskp != 0)
			*maskp = strdup(myhostinfo.netmask);

		return 0;
	}

	return err;
}

int
start_tmcp_getinfo_host(char **hnamep, char **dnamep, char **gwnamep,
			char ***nsnamesp)
{
#define SETIT(p, f) \
	if (p != 0) *(p) = myhostinfo.f ? strdup(myhostinfo.f) : 0;

	SETIT(hnamep, name);
	SETIT(dnamep, domain);
	SETIT(gwnamep, gateway);
	if (nsnamesp != 0) {
		*nsnamesp = (char **)malloc(sizeof(char *) * 2);
		SETIT(*nsnamesp, nameserver);
		(*nsnamesp)[1] = 0;
	}

	return 0;

#undef SETIT
}
