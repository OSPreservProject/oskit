/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
#include <string.h>          /* for memset */
#include <stdlib.h>       
#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <oskit/dpf.h>

#include <assert.h>

void
mk_ip(char *ip_addr, unsigned char ip1, unsigned char ip2, 
      unsigned char ip3, unsigned char ip4)
{
	ip_addr[0] = ip1;
	ip_addr[1] = ip2;
	ip_addr[2] = ip3;
	ip_addr[3] = ip4;
}

/* 
 * Create an ARP filter with IP address 16.32.0.0, and 
 * verify that the filter is working with a fake ARP packet.
 */
void
main(int argc, char *argv[])
{
	int pid, got = 0;
	unsigned char msg[1024];
	char  ip_addr[8];
	void *filter;
	int sz;

	oskit_clientos_init();
	oskit_dpf_init();

	/* initialize the ip address */
	memset(ip_addr, 0, sizeof ip_addr);
	mk_ip(ip_addr, 16, 32, 0, 0);

	/* Create an ARP filter template (the IP address does nothing) */
	filter = oskit_dpf_mk_arp_req(&sz, ip_addr);
	
	printf("------ filter is %d bytes.\n", sz);

	/* insert the filter */
	if ((pid = oskit_dpf_insert(filter, sz)) < 0)
		assert(0); /* "bogus insertion" */

	/* make a fake arp message we can use to test the filter*/
	oskit_dpf_mk_arp_rep(msg, ip_addr);

	got = oskit_dpf_iptr(msg, 1024);
	if (got != pid) {
		printf("------ got pid %d, %d, expecting %d\n", got, oskit_dpf_iptr(msg, 1024), pid);
	} else {
		printf("------ got correct pid %d\n", got);
	}
}

