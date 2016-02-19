/*
 * Copyright (c) 1998 The University of Utah and the Flux Group.
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
 * Interface to the Fake UDP library, which provides send-only unfragmented
 * datagram service.
 */

#ifndef _OSKIT_FUDP_H_
#define _OSKIT_FUDP_H_

#include <oskit/net/socket.h>
#include <oskit/c/netinet/in.h>
#include <oskit/net/ether.h>

/*
 * The main entry point.
 * Gives you a socket factory which will manufacture send-only UDP
 * sockets for you.
 */
oskit_error_t fudp_init(oskit_socket_factory_t **out_factory);

/*
 * What fake UDP implementation would be complete without a
 * corresponding fake ARP implementation?
 */
oskit_error_t farp_lookup(const struct in_addr *ipaddr,
			 unsigned char out_ethaddr[ETHER_ADDR_SIZE]);
oskit_error_t farp_add(const struct in_addr *ipaddr,
		      const unsigned char ethaddr[ETHER_ADDR_SIZE]);
void farp_remove(const struct in_addr *ipaddr);

#endif /* _OSKIT_FUDP_H_ */
