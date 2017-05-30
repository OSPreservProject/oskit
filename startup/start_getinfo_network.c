/*
 * Copyright (c) 1997-1999, 2001 University of Utah and the Flux Group.
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
 * start_getinfo_network.c
 *
 * Get default network information for a host and its interfaces.
 * Interfaces are configured first via BOOTP, then any of the appropriate
 * environment variables are checked to supply values, finally for required
 * info, the user is prompted.
 *
 * Environment variables override BOOTP-provided information.  The
 * first BOOTP server to provide a host-level value overrides
 * subsequent BOOTP values.  The host-level values are: hostname,
 * gateway, nameserver(s), and domain.  The per-interface information
 * is address and netmask.
 *
 * Host-level environment variables include:
 *  start_hostname
 *  start_domainname
 *  start_gateway
 *  start_nameservers
 * Interface environment variables include:
 *  start_<name>_addr  (i.e, start_de0_addr)
 *  start_<name>_mask
 */
#include <oskit/startup.h>
#include <oskit/dev/error.h>
#include <oskit/net/bootp.h>

#include <oskit/c/arpa/inet.h>
#include <oskit/c/assert.h>
#include <oskit/c/stdarg.h>
#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>
#include <oskit/c/unistd.h>

#define VERBOSITY 0

extern long secondswest; /* from start_clock.c */

/*
 * Host-wide info
 */
#define NAMELEN	64
static char *hostname;
static char *domainname;
static char *gateway;
static char **nameservers;

static int query_user_netaddr(char *result, int maxlen, const char *fmt, ...);
static int query_user_string(char *result, int maxlen, const char *fmt, ...);

/*
 * Sets machine-wide (host) network parameters using a variety of sources
 * for default values.  Returns zero if all is well, an error otherwise.
 */
int
start_getinfo_network_host(char **hnamep, char **dnamep, char **gwnamep,
			   char ***nsnamesp, int ask)
{
	int err = 0;

	assert(hnamep != 0 && dnamep != 0 && gwnamep != 0 && nsnamesp != 0);

	if (getenv("start_hostname")) {
		if (hostname != NULL)
			free(hostname);
		hostname = strdup(getenv("start_hostname"));
	}

	if (getenv("start_domainname")) {
		if (domainname != NULL)
			free(domainname);
		domainname = strdup(getenv("start_domainname"));
	}

	if (getenv("start_gateway")) {
		if (gateway != NULL)
			free(gateway);
		gateway = strdup(getenv("start_gateway"));
	}

	if (getenv("start_nameservers")) {
		char **ns, *cp, *ocp;
		int i, nslen;;
		
		if (nameservers != NULL) {
			ns = nameservers;
			while (*ns != NULL) {
				free(*ns);
				ns++;
			}
			free(nameservers);
		}

		nslen = 1;
		ocp = cp = strdup(getenv("start_nameservers"));
		while (*cp != '\0') {
			if (*cp == ',')
				nslen++;
			cp++;
		}
		cp = ocp;

		nameservers = malloc(sizeof(char *) * (nslen + 1));
		for (i = 0; i < nslen; i++)
			nameservers[i] = strsep(&cp, ",");
		nameservers[i] = NULL;
	}

	/*
	 * Ask the user for host-level configuration data if
	 * none is provided.
	 */
	if (ask) {
		if (err == 0 && hostname == NULL) {
			hostname = malloc(NAMELEN);
			err = query_user_string(hostname, NAMELEN,
						"Hostname for this machine");
		}

		if (err == 0 && domainname == NULL) {
			domainname = malloc(NAMELEN);
			err = query_user_string(domainname, NAMELEN,
						"Domain name for this machine");
		}

		if (err == 0 && gateway == NULL) {
			gateway = malloc(NAMELEN);
			err = query_user_netaddr(gateway, NAMELEN,
						 "IP address of gateway "
						 "for this machine");
		}

		if (err == 0 && nameservers == NULL) {
			nameservers = malloc(sizeof(char *) * 2);
			nameservers[0] = malloc(NAMELEN);
			nameservers[1] = NULL;
			err = query_user_netaddr(nameservers[0], NAMELEN,
						 "IP address of nameserver "
						 "for this machine");
			
			/* XXX we can live without a nameserver */
			if (err != 0) {
				free(nameservers[0]);
				free(nameservers);
				nameservers = NULL;
				err = 0;
			}
		}
	}

	if (err == 0) {
		*hnamep = hostname;
		*dnamep = domainname;
		*gwnamep = gateway;
		*nsnamesp = nameservers;
	}

	return err;
}

int
start_getinfo_network_eif(int ifn, char **addrp, char **maskp, int ask)
{
	struct bootp_net_info bootpinfo;
	char start_eif_addr_var[32];
	char start_eif_mask_var[32];
	oskit_etherdev_t **edev = NULL;
	char *eif_addr = NULL;
	char *eif_mask = NULL;
	const char *eif_name;
	int ndev, nif, err;

	assert(addrp != 0 && maskp != 0);

	ndev = osenv_device_lookup(&oskit_etherdev_iid, (void***)&edev);
	if (ndev <= 0 || ifn >= ndev) {
		err = OSKIT_EINVAL;
		goto done;
	}

	eif_name = start_conf_network_eifname(ifn);
	if (eif_name == NULL) {
		err = OSKIT_EINVAL;
		goto done;
	}

	snprintf(start_eif_addr_var, sizeof start_eif_addr_var,
		 "start_%s_addr", eif_name);
	snprintf(start_eif_mask_var, sizeof start_eif_mask_var,
		 "start_%s_mask", eif_name);
			
#if VERBOSITY > 1
	printf("Getting info about dev %d(%s)\n", ifn, eif_name);
#endif

	/*
	 * Grab BOOTP info if available
	 */
	err = bootp(edev[ifn], &bootpinfo);
	if (err == 0) {
#if VERBOSITY > 4
		bootp_dump(&bootpinfo);
#endif
		/* Grab the interface address */
		if (bootpinfo.flags & BOOTP_NET_IP)
			eif_addr = strdup(inet_ntoa(bootpinfo.ip));

		/* Grab the interface network mask. */
		if (bootpinfo.flags & BOOTP_NET_NETMASK)
			eif_mask = strdup(inet_ntoa(bootpinfo.netmask));
		else if (bootpinfo.flags & BOOTP_NET_IP)
			eif_mask = strdup(inet_ntoa(
					bootp_default_netmask(bootpinfo.ip)));

		/* Grab the physical location of the machine. */
		if (bootpinfo.flags & BOOTP_NET_TIME_OFFSET)
			secondswest = bootpinfo.time_offset;

		/*
		 * For the host-level data, only record it if
		 * it isn't already known.
		 */
		if (hostname == NULL && (bootpinfo.flags & BOOTP_NET_HOSTNAME))
			hostname = strdup(bootpinfo.hostname);

		if (domainname == NULL &&
		    (bootpinfo.flags & BOOTP_NET_DOMAINNAME))
			domainname = strdup(bootpinfo.domainname);
			
		/* XXX only record one gateway from the N provided */
		if (gateway == NULL && (bootpinfo.flags & BOOTP_NET_GATEWAY))
			gateway = strdup(inet_ntoa(bootpinfo.gateway.addr[0]));

		if (nameservers == NULL &&
		    (bootpinfo.flags & BOOTP_NET_DNS_SERVER)) {
			struct bootp_addr_array ns;
			int x;

			ns = bootpinfo.dns_server;
			nameservers = malloc((sizeof(char *) * ns.len) + 1);
			for (x = 0; x < ns.len; x++)
				nameservers[x] = strdup(inet_ntoa(ns.addr[x]));
			nameservers[ns.len] = NULL;
		}

		bootp_free(&bootpinfo);
	}

	err = 0;

	/*
	 * Override per-interface BOOTP info with environment variables,
	 * if they're set.
	 */
	if (getenv(start_eif_addr_var)) {
		if (eif_addr != NULL)
			free(eif_addr);
		eif_addr = strdup(getenv(start_eif_addr_var));
	}
	if (getenv(start_eif_mask_var)) {
		if (eif_mask != NULL)
			free(eif_mask);
		eif_mask = strdup(getenv(start_eif_mask_var));
	}
		
	/*
	 * If either the address or netmask are unavailable,
	 * query the user for them.
	 */
	if (ask) {
		if (err == 0 && eif_addr == NULL) {
			eif_addr = malloc(sizeof(char) * NAMELEN);
			err = query_user_netaddr(eif_addr, NAMELEN,
						 "IP address for %s (eif# %d)",
						 eif_name, ifn);
		}
		
		if (err == 0 && eif_mask == NULL) {
			eif_mask = malloc(sizeof(char) * NAMELEN);
			err = query_user_netaddr(eif_mask, NAMELEN,
						 "Netmask for %s (eif# %d)",
						 eif_name, ifn);
		}
	}
		
 done:
	if (edev != 0) {
		assert(ndev > 0);
		for (nif = 0; nif < ndev; nif++)
			oskit_etherdev_release(edev[nif]);
		free(edev);
	}

	if (err == 0) {
		*addrp = eif_addr;
		*maskp = eif_mask;
	} else {
		if (eif_addr != NULL)
			free(eif_addr);
		if (eif_mask != NULL)
			free(eif_mask);
	}

	return err;
}

#define EXITSTR "@"

/*
 * Ask the user (via the console) for configuration information
 * XXX if they type "exit" we abort.
 */
static int
query_user_netaddr(char *result, int maxlen, const char *fmt, ...)
{
	struct in_addr ip;
	va_list args;
	int err = 0;
		
        va_start(args, fmt);
	do {
		vprintf(fmt, args);
		printf("(\"%s\" to abort): ", EXITSTR);
		fgets(result, maxlen, stdin);
		result[strlen(result)-1] = '\0'; /* Nuke trailing \n */
		if (strcmp(result, EXITSTR) == 0)
			err = -1;
	} while(err == 0 && inet_aton(result, &ip) == 0);
        va_end(args);

	return err;
}

static int
query_user_string(char *result, int maxlen, const char *fmt, ...)
{
	va_list args;
	int err = 0;
		
        va_start(args, fmt);

	vprintf(fmt, args);
	printf("(\"%s\" to abort): ", EXITSTR);
	fgets(result, maxlen, stdin);
	result[strlen(result)-1] = '\0'; /* Nuke trailing \n */
	if (strcmp(result, EXITSTR) == 0)
		err = -1;

        va_end(args);
	return err;
}
