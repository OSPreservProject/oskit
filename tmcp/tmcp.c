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

/*
 * emulab.net "Testbed Master Control" protocol support.
 */

#undef DUMPTMCPINFO	/* dump received info to console */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <oskit/error.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/dev.h>
#include <oskit/net/ether.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <oskit/tmcp.h>
#include "tmcp.h"

struct interfaces {
	short tbnum;
	short oskitnum;
	struct in_addr myip;
	struct in_addr mask;
	struct in_addr alias;
	unsigned char mac[ETHER_ADDR_SIZE];
};

struct hosts {
	char *name;
	char *alias;
	struct in_addr ip;
};

static struct interfaces *interfaces;
static int ninterfaces;
static struct hosts *hosts;
static int nhosts;

static int tmcp_makehosts(char *str);
static int tmcp_makeinterfaces(char *str);
static char *mactostr(unsigned char ea[]);

/*
 * Public interfaces
 */

/*
 * Using TMCP provided info, map the provided host name (sans domain)
 * into an IP address.  If localonly is non-zero it looks for the host
 * name exactly and returns a directly reachable address.  Otherwise it
 * looks for an alias for the host on another network in the experiment
 * (i.e., reachable via caller provided routing).  If the routine returns
 * zero, the address is returned in *addr.  Non-zero is returned if the
 * hostname didn't match any TMCP provided info.
 */
int
tmcp_name2addr(char *host, struct in_addr *addr, int localonly)
{
	int i;

	for (i = 0; i < nhosts; i++) {
		if (strcmp(host, hosts[i].name) == 0 ||
		    (hosts[i].alias && strcmp(host, hosts[i].alias) == 0)) {
			*addr = hosts[i].ip;
			return 0;
		}
	}

	/*
	 * XXX routing hack.
	 *
	 * If so desired, see if we can find a <host>-0, which is one of
	 * possibly many names for the host on other networks.  If we find it,
	 * then return that IP address.
	 */
	if (!localonly) {
		char *nhost;
		int err;

		nhost = malloc(strlen(host)+2);
		sprintf(nhost, "%s-0", host);
		err = tmcp_name2addr(nhost, addr, 0);
		free(nhost);
		return err;
	}

	return OSKIT_EINVAL;
}

/*
 * Using TMCP provided info, map the provided IP address to an appropriate
 * interface (taking into account the netmask for the interface).
 * *oskitifn is non-zero on call, that value is returned as a default if
 * no interfaces were configured with TMCP.  If the routine returns zero,
 * *oskitifn contains the OSKit interface number (as ordered by
 * osenv_device_lookup) of the appropriate interface.  Returns non-zero
 * if no matching TMCP-confgiured interface was found.
 */
int
tmcp_addr2if(struct in_addr *addr, int *ifn)
{
	int i;

	for (i = 0; i < ninterfaces; i++) {
		unsigned long hnet = (addr->s_addr & interfaces[i].mask.s_addr);

		if (hnet ==
		    (interfaces[i].myip.s_addr & interfaces[i].mask.s_addr) ||
		    hnet == 
		    (interfaces[i].alias.s_addr & interfaces[i].mask.s_addr)) {
			*ifn = interfaces[i].oskitnum;
			return 0;
		}
	}

	/*
	 * XXX no TMCD info or IP not found but user specified an interface,
	 * return what they specified.
	 */
	if (*ifn >= 0)
		return 0;

	return OSKIT_EINVAL;
}

/*
 * Using TMCP provided info, map the provided OSKit interface number
 * (as ordered by osenv_device_lookup) to an IP address and mask for
 * that interface.  If the routine returns zero, *addr and *mask contain
 * the TMCP configured IP address/mask of that interface.  Returns non-zero
 * if that interface is not in the TMCP-provided info.
 */
int
tmcp_if2addr(int ifn, struct in_addr *addr, struct in_addr *mask)
{
	int i;

	for (i = 0; i < ninterfaces; i++) {
		if (ifn == interfaces[i].oskitnum) {
			/* use the alias if it exists */
			if (interfaces[i].alias.s_addr != 0)
				*addr = interfaces[i].alias;
			else
				*addr = interfaces[i].myip;
			if (mask)
				*mask = interfaces[i].mask;
			return 0;
		}
	}

	return OSKIT_EINVAL;
}

/*
 * Map the provided OSKit interface number (as ordered by osenv_device_lookup)
 * to the MAC address for the interface.  If the routine returns zero, *mac
 * contains the address of the interface.  Returns non-zero if that interface
 * does not exist.  Note that this doesn't require TMCP info right now,
 * but could someday if we every had a way to virtualize MAC addresses.
 */
int tmcp_if2mac(int ifn, unsigned char mac[6])
{
	int i;

	for (i = 0; i < ninterfaces; i++) {
		if (ifn == interfaces[i].oskitnum) {
			memcpy(mac, interfaces[i].mac, sizeof mac);
			return 0;
		}
	}

	return OSKIT_EINVAL;
}

/*
 * Inform TMCD that the node is shutting down.
 * The indicated status value is returned to TMCD to report to the
 * experiment creator.  If sendlog is non-zero, any pending log information
 * (written via tmcp_vlog) is sent back to TMCD.
 *
 * This should be the last tmcp_* function called (duh!)
 */
void
tmcp_shutdown(int status, int sendlog)
{
	int err, len;
	char tmpbuf[32];
	static int called;

	if (called++ > 0)
		return;

	err = tmcp_netrestart();
	if (err != 0)
		goto error;

	sprintf(tmpbuf, "startstat %d", 0x30ab);
	len = 0;
	err = tmcp_sendmsg(tmpbuf, &len);
	if (err != 0)
		goto error;

	/*
	 * Send the log if desired (and not empty)
	 */
	if (sendlog)
		err = tmcp_logdump();

 error:
	tmcp_netshutdown();
	if (err != 0)
		printf("WARNING: could not send shutdown to TMCD\n");
	return;
}

/*
 * Dump all state obtained via TMCP.
 */
void
tmcp_dumpinfo(void)
{
	int i;

	printf("Host info:\n");
	for (i = 0; i < nhosts; i++) {
		printf("  name=%s inet=%s",
		       inet_ntoa(hosts[i].ip), hosts[i].name);
		if (hosts[i].alias != 0 && hosts[i].alias[0] != 0)
			printf(" alias=%s", hosts[i].alias);
		printf("\n");
	}

	printf("Interface info:\n");
	for (i = 0; i < ninterfaces; i++) {
		printf("  tb_if=%d oskit_if=%d mac=%s inet=%s ",
		       interfaces[i].tbnum, interfaces[i].oskitnum,
		       mactostr(interfaces[i].mac),
		       inet_ntoa(interfaces[i].myip));
		printf("mask=%s", inet_ntoa(interfaces[i].mask));
		if (interfaces[i].alias.s_addr != 0)
			printf(" alias=%s", inet_ntoa(interfaces[i].alias));
		printf("\n");
	}
}


/*
 * Internal interfaces
 */

int
tmcp_getinfo(oskit_etherdev_t *edev, int myif, struct in_addr *myip,
	     struct in_addr *nextip, struct in_addr *tmcp_host,
	     int tmcp_port, int waitforall)
{
	int err, len;
	static char tmpbuf[8192];

	err = tmcp_netstart(edev, myif, myip, nextip, tmcp_host, tmcp_port);
	if (err)
		return err;

	/*
	 * Now send requests for all the info we want.
	 */
	len = sizeof(tmpbuf);
	strcpy(tmpbuf, "hostnames");
	err = tmcp_sendmsg(tmpbuf, &len);
	if (err == 0) {
		tmpbuf[len] = 0;
		tmcp_makehosts(tmpbuf);
	} else
		printf("WARNING: could not get TMCD hostnames, err=%x\n", err);

	len = sizeof(tmpbuf);
	strcpy(tmpbuf, "ifconfig");
	err = tmcp_sendmsg(tmpbuf, &len);
	if (err == 0) {
		tmpbuf[len] = 0;
		tmcp_makeinterfaces(tmpbuf);
	} else
		printf("WARNING: could not get TMCD ifconfig, err=%x\n", err);

	/*
	 * Inform the server that we are ready to go.
	 *
	 * We really aren't quite ready, but it is better to do this
	 * now so we don't interfere with the apps use of the interface
	 * in the case that the experiment interface is the same as the
	 * control interface (sharks).
	 */
	len = 0;
	strcpy(tmpbuf, "ready");
	err = tmcp_sendmsg(tmpbuf, &len);
	if (err != 0) {
		printf("WARNING: could not sent TMCD ready, err=%x\n", err);
		if (waitforall) {
			printf("WARNING: cannot wait on readycount\n");
			waitforall = 0;
		}
	}

	/*
	 * If requested, wait for all other nodes in the experiment
	 * to come ready.
	 */
	while (waitforall != 0) {
		int num, ready, total;

		len = sizeof(tmpbuf);
		strcpy(tmpbuf, "readycount");
		err = tmcp_sendmsg(tmpbuf, &len);
		if (err != 0) {
			printf("WARNING: "
			       "could not get TMCD readycount, err=%x\n", err);
			break;
		}

		tmpbuf[len] = 0;
		num = sscanf(tmpbuf, "READY=%d TOTAL=%d", &ready, &total);
#ifdef DUMPTMCPINFO
		if (num == 2)
			printf("ready=%d, total=%d\n", ready, total);
#endif
		if (num != 2 || ready < total) {
			for (num = 0; num < 5; num++)
				osenv_timer_spin(1000000000); /* 1 sec */
		} else
			waitforall = 0;
	}

	tmcp_netstop();
	return 0;
}

void
tmcp_mapifs(oskit_etherdev_t **edevs, int nedevs)
{
	int i, j;
	unsigned char ea[ETHER_ADDR_SIZE];

	for (j = 0; j < ninterfaces; j++)
		interfaces[j].oskitnum = -1;

	for (i = 0; i < nedevs; i++) {
		oskit_etherdev_getaddr(edevs[i], ea);
		for (j = 0; j < ninterfaces; j++) {
			if (!memcmp(ea, interfaces[j].mac, ETHER_ADDR_SIZE)) {
				interfaces[j].oskitnum = i;
#ifdef DUMPTMCPINFO
				printf("tbeth%d -> OSkit eth%d\n",
				       interfaces[j].tbnum, i);
#endif
				break;
			}
		}
	}

	for (j = 0; j < ninterfaces; j++)
		if (interfaces[j].oskitnum == -1)
			printf("WARNING: TMCP eth%d MAC address %s "
			       "doesn't map to this machine\n",
			       interfaces[j].tbnum,
			       mactostr(interfaces[j].mac));
}

#ifdef USE_FREEBSD_CLIB
static char hostrecord[] =
	"NAME=%63s LINK=%d IP=%16s ALIAS=%63s";
static char ifrecord[] =
	"INTERFACE=%d INET=%16s MASK=%16s MAC=%16s";
static char ifwaliasrecord[] =
	"INTERFACE=%d INET=%16s MASK=%16s IPALIAS=%16s MAC=%16s";
#else
static char hostrecord[] =
	"NAME=%s LINK=%d IP=%s ALIAS=%s";
static char ifrecord[] =
	"INTERFACE=%d INET=%s MASK=%s MAC=%s";
static char ifwaliasrecord[] =
	"INTERFACE=%d INET=%s MASK=%s IPALIAS=%s MAC=%s";
#endif

/*
 * Parse the hacky TMCD mesages and construct interface and hosts tables.
 */
static int
tmcp_makehosts(char *str)
{
	char *cp;
	int num, i, link;
	char nbuf[64], abuf[64], ipbuf[20];

	num = 0;
	for (cp = str; (cp = strchr(cp, '\n')) != 0; cp++)
		num++;
	hosts = malloc(sizeof(struct hosts) * num);
	if (hosts == 0)
		return -1;

	memset(hosts, 0, sizeof(struct hosts) * num);
	for (i = 0; (cp = strsep(&str, "\n")) != 0;) {
		num = sscanf(cp, hostrecord, nbuf, &link, ipbuf, abuf);
		if (num < 3)
			continue;
		hosts[i].name = malloc(strlen(nbuf)+5);
		if (hosts[i].name)
			sprintf(hosts[i].name, "%s-%d", nbuf, link);
		if (num == 4) {
			hosts[i].alias = malloc(strlen(abuf)+1);
			if (hosts[i].alias)
				strcpy(hosts[i].alias, abuf);
		}
		inet_aton(ipbuf, &hosts[i].ip);
#ifdef DUMPTMCPINFO
		printf("%s  %s %s\n", inet_ntoa(hosts[i].ip),
		       hosts[i].name, hosts[i].alias);
#endif
		i++;
	}

	nhosts = i;
	return 0;
}

static int
tmcp_makeinterfaces(char *str)
{
	char *cp, *mbp;
	int num, i, j, ifn;
	char ipbuf[20], maskbuf[20], aliasbuf[20], macbuf[20];

	num = 0;
	for (cp = str; (cp = strchr(cp, '\n')) != 0; cp++)
		num++;

	/*
	 * XXX leave an extra entry for the control net in case TMCD
	 * doesn't supply the info.
	 */
	num++;

	interfaces = malloc(sizeof(struct interfaces) * num);
	if (interfaces == 0)
		return -1;

	memset(interfaces, 0, sizeof(struct interfaces) * num);
	for (i = 0; (cp = strsep(&str, "\n")) != 0;) {
		if (strstr(cp, "IPALIAS") != 0)
			num = sscanf(cp, ifwaliasrecord,
				     &ifn, ipbuf, maskbuf, aliasbuf, macbuf);
		else
			num = sscanf(cp, ifrecord,
				     &ifn, ipbuf, maskbuf, macbuf);
		if (num < 4)
			continue;
		interfaces[i].tbnum = ifn;
		inet_aton(ipbuf, &interfaces[i].myip);
		inet_aton(maskbuf, &interfaces[i].mask);
		mbp = macbuf;
		for (j = 0; j < ETHER_ADDR_SIZE; j++) {
			char tmp = mbp[2];
			mbp[2] = 0;
			interfaces[i].mac[j] = strtol(mbp, 0, 16);
			mbp[2] = tmp;
			mbp += 2;
		}
		if (num == 5)
			inet_aton(aliasbuf, &interfaces[i].alias);
#ifdef DUMPTMCPINFO
		printf("tbeth%d mac=%s inet=%s ",
		       interfaces[i].tbnum, mactostr(interfaces[i].mac),
		       inet_ntoa(interfaces[i].myip));
		printf("mask=%s", inet_ntoa(interfaces[i].mask));
		if (interfaces[i].alias.s_addr != 0)
			printf(" alias=%s", inet_ntoa(interfaces[i].alias));
		printf("\n");
#endif

		i++;
	}

	ninterfaces = i;
	return 0;
}

/*
 * Utility
 */
static char *
mactostr(unsigned char ea[])
{
	static char macbuf[18];

	sprintf(macbuf, "%02x:%02x:%02x:%02x:%02x:%02x",
		ea[0], ea[1], ea[2], ea[3], ea[4], ea[5]);

	return macbuf;
}
