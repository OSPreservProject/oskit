/*
 * Copyright (c) 1998, 2001 The University of Utah and the Flux Group.
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
 * Our fake ARP service.
 * This doesn't really do any ARP protocol stuff (I said it was fake didn't I?)
 * but provides a simple farp_add/lookup/delete service.
 */

#include <oskit/error.h>
#include <oskit/net/ether.h>
#include <oskit/io/bufio.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <oskit/net/ether.h>
#include <oskit/net/arp.h>

#include <oskit/udplib.h>
#include "udplib.h"

struct arp_mapping {
	struct in_addr	am_ip;
	unsigned char	am_eth[ETHER_ADDR_SIZE];
};

#define TABLE_SIZE 100
static struct arp_mapping arptab[TABLE_SIZE];

/*
 * Lookup the Ethernet address for an IP address.
 */
oskit_error_t
farp_lookup(const struct in_addr *ipaddr,
	    unsigned char out_ethaddr[ETHER_ADDR_SIZE])
{
	int i;

	for (i = 0; i < TABLE_SIZE; i++)
		if (arptab[i].am_ip.s_addr == ipaddr->s_addr) {
			memcpy(out_ethaddr, arptab[i].am_eth, ETHER_ADDR_SIZE);
			return 0;
		}
	return OSKIT_E_FAIL;
}

/*
 * Add (or replace) a mapping for an IP address.
 */
oskit_error_t
farp_add(const struct in_addr *ipaddr,
	 const unsigned char ethaddr[ETHER_ADDR_SIZE])
{
	int i;

	for (i = 0; i < TABLE_SIZE; i++)
		if (arptab[i].am_ip.s_addr == 0 ||
		    arptab[i].am_ip.s_addr == ipaddr->s_addr) {
			arptab[i].am_ip.s_addr = ipaddr->s_addr;
			memcpy(arptab[i].am_eth, ethaddr, ETHER_ADDR_SIZE);
			return 0;
		}
	return OSKIT_E_FAIL;
}

/*
 * Removes all mappings for a particular IP address.
 */
void
farp_remove(const struct in_addr *ipaddr)
{
	int i;

	for (i = 0; i < TABLE_SIZE; i++)
		if (arptab[i].am_ip.s_addr == ipaddr->s_addr)
			arptab[i].am_ip.s_addr = 0;
}

