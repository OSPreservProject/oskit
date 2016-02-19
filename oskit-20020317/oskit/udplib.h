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
 * Interface to the simple UDP library, which provides unfragmented
 * datagram service. 
 */

#ifndef _OSKIT_UDPLIB_H_
#define _OSKIT_UDPLIB_H_

#include <oskit/net/socket.h>
#include <netinet/in.h>
#include <oskit/net/ether.h>
#include <oskit/dev/ethernet.h>

/*
 * The main entry point.
 * Gives you a socket factory which will manufacture UDP sockets for you.
 */
oskit_error_t
udplib_init(oskit_etherdev_t *dev, char *name,
	    char *ipaddr, char *netmask, char *gateway,
	    oskit_socket_factory_t **out_factory);

#endif /* _OSKIT_UDPLIB_H_ */
