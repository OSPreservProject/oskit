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

/*
 * start_conf_network_host.c
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <oskit/startup.h>
#include <oskit/clientos.h>
#include <oskit/net/freebsd.h>

static void
write_dns_files(const char *domainname, char **nameservers);

int
start_conf_network_host(const char *hostname, const char *gateway,
			const char *domainname, char **nameservers)
{
	int err;

	if (hostname == NULL || domainname == NULL)
		return OSKIT_EINVAL;

	oskit_clientos_sethostname(hostname, strlen(hostname));
	write_dns_files(domainname, nameservers);

	if (gateway != NULL) {
		err = oskit_freebsd_net_add_default_route((char *)gateway);
		if (err != 0)
			printf("WARNING: could not add default route to %s "
			       "(err %d: `%s')\n", gateway, err, strerror(err));
	} else
		printf("WARNING: could not add default route, "
		       "no gateway defined\n");

	return 0;
}

static void
write_dns_files(const char *domainname, char **nameservers)
{
	int have_hosts, have_host_conf, have_resolv_conf;
	int host_conf;
	int resolv_conf;
	

	if (mkdir("/etc", 0777) < 0 && errno != EEXIST) {
		perror("mkdir /etc");
		return;
	}

	have_hosts = (access("/etc/hosts", R_OK) == 0);
	have_host_conf = (access("/etc/host.conf", R_OK) == 0);
	have_resolv_conf = (access("/etc/resolv.conf", R_OK) == 0);
	
	if (!have_hosts && !have_resolv_conf &&
	    (nameservers == NULL || nameservers[0] == NULL)) {
		printf("WARNING: no method for resolving host names, "
		       "no nameserver or /etc/hosts file\n");
		return;
	}
	
	/*
	 * Don't clobber any existing host.conf or resolv.conf,
	 * this might be a "real" system!
	 */
	if (!have_host_conf) {
		host_conf = open("/etc/host.conf",
				 O_RDWR|O_CREAT|O_TRUNC|O_EXCL, 0644);
		if (host_conf >= 0) {
			if (have_resolv_conf || nameservers != NULL)
				write(host_conf, "bind\n", strlen("bind\n"));

			if (have_hosts)
				write(host_conf, "hosts\n", strlen("hosts\n"));

			close(host_conf);
		} else
			perror("couldn't create /etc/host.conf");
	}

	if (!have_resolv_conf) {
		resolv_conf = open("/etc/resolv.conf",
				   O_RDWR|O_CREAT|O_TRUNC|O_EXCL, 0644);
		if (resolv_conf >= 0) {
			if (domainname != NULL) {
				write(resolv_conf, "search\t",
				      strlen("search\t"));
				write(resolv_conf, domainname,
				      strlen(domainname));
				write(resolv_conf, "\n", 1);
			}
			if (nameservers != NULL) {
				char **ns = nameservers;

				while ((*ns) != NULL) {
					write(resolv_conf, "nameserver\t",
					      strlen("nameserver\t"));
					write(resolv_conf, *ns, strlen(*ns));
					write(resolv_conf, "\n", 1);
					ns++;
				}
			}
			close(resolv_conf);
		} else
			perror("couldn't create /etc/resolv.conf");
	}
}
