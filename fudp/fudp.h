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
#ifndef _FUDP_FUDP_H_
#define _FUDP_FUDP_H_

#include <oskit/io/netio.h>
#include <oskit/io/bufio.h>
#include <oskit/c/netinet/in.h>
#include <oskit/net/ether.h>

oskit_error_t ip_send(oskit_netio_t *netio,
		      const struct in_addr *from,
		      const struct in_addr *to,
		      oskit_bufio_t *b,
		      oskit_size_t blen);

oskit_error_t eth_transmit(oskit_netio_t *netio,
			   unsigned char src[],
			   unsigned char dst[],
			   unsigned short type,
			   oskit_bufio_t *b,
			   oskit_size_t blen);

#endif /* _FUDP_FUDP_H_ */
