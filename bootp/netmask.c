/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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

#include <oskit/types.h>
#include <oskit/c/string.h>
#include <oskit/c/arpa/inet.h>
#include <oskit/c/netinet/in.h>
#include <oskit/net/bootp.h>

/*
 * Returns the default netmask for a given IPv4 address (in host order)
 */
struct in_addr
bootp_default_netmask(struct in_addr addr)
{
	struct in_addr ret;

	memset(&ret, 0, sizeof ret);
	if (IN_CLASSA(ntohl(addr.s_addr)))
		ret.s_addr = htonl(IN_CLASSA_NET);

	if (IN_CLASSB(ntohl(addr.s_addr)))
		ret.s_addr = htonl(IN_CLASSB_NET);

	if (IN_CLASSC(ntohl(addr.s_addr)))
		ret.s_addr = htonl(IN_CLASSC_NET);

	if (IN_CLASSD(ntohl(addr.s_addr)))
		ret.s_addr = htonl(IN_CLASSD_NET);

	return ret;
}

