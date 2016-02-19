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

#include <stdio.h>
#include <stdlib.h>
#include <oskit/startup.h>

#ifdef	PTHREADS
#include <oskit/threads/pthread.h>

#define start_network_single	start_network_single_pthreads
#endif

/*
 * Start the network using the indicated single interface.
 */
void
start_network_single(int ifn)
{
	int err;
	char *addr, *mask;
	char *host, *domain, *gateway, **nameservers;
	oskit_socket_factory_t *fsc;
	static int called;

	if (!called) {
		called = 1;
		err = start_conf_network_init(start_osenv(), &fsc);
		if (err != 0) {
			printf("WARNING: could not initialize network\n");
			return;
		}
#ifdef  PTHREADS
		pthread_init_socketfactory(fsc);
#else
		oskit_register(&oskit_socket_factory_iid, (void *)fsc);
#endif
	}

	err = start_getinfo_network_eif(ifn, &addr, &mask, 1);
	if (err == 0) {
		err = start_conf_network_eifstart(ifn, addr, mask);
		free(addr);
		free(mask);
	}
	if (err != 0)
		printf("WARNING: could not initialize eif# %d\n", ifn);

	err = start_getinfo_network_host(&host, &domain, &gateway,
					 &nameservers, 1);
	if (err == 0) {
		err = start_conf_network_host(host, gateway, domain,
					      nameservers);
		free(host);
		free(gateway);
		free(domain);
		if (nameservers)
			free(nameservers);
	}
	if (err != 0)
		printf("WARNING: could not initialize host information\n");
}


