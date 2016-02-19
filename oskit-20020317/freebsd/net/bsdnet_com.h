/*
 * Copyright (c) 2001 University of Utah and the Flux Group.
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
 * Declarations for the COM objects that provide interfaces to the FreeBSD
 * networking stack.
 */
#ifndef _OSKIT_FREEBSD_NET_BSDNET_COM_H
#define _OSKIT_FREEBSD_NET_BSDNET_COM_H

#include <oskit/net/socket.h>
#if FLASK
#  include <oskit/net/socket_secure.h>
#endif
#include <oskit/mib/ip.h>
#include <oskit/mib/icmp.h>
#include <oskit/mib/tcp.h>
#include <oskit/mib/udp.h>

/*****************************************************************************/

typedef struct bsdnet_impl bsdnet_impl_t;
struct bsdnet_impl {
	/* Our socket factory interface.  See <oskit/net/socket.h>. */
	oskit_socket_factory_t	sf;
	
	/* Our monitor/control interfaces.  See <oskit/mib/---.h>. */
	oskit_mib_ip_t		ip_mib;		/* IP MIB interface    */
	oskit_mib_icmp_t	icmp_mib;	/* ICMP MIB interface  */
	oskit_mib_tcp_t		tcp_mib;	/* TCP MIB interface   */
	oskit_mib_udp_t		udp_mib;	/* UDP MIB interface   */
};

/* The object that provides all of the COM interfaces to the FreeBSD net. */
extern bsdnet_impl_t oskit_freebsd_net_impl;

/*****************************************************************************/

#if FLASK

typedef struct bsdnet_secure_impl bsdnet_secure_impl_t;
struct bsdnet_secure_impl {
	/* Our secure socket factory intf.  See <oskit/net/socket_secure.h>. */
	oskit_socket_factory_secure_t	ssf;
	
	/* XXX --- No MIB interfaces for now. */
	/*
	 * I suppose that this implementation should be MIB-ified in the same
	 * way that `bsdnet_impl_t' has been.  But I don't know if there are
	 * security implications for this, or what they are.
	 */
};

/* The object that provides the secure COM interfaces to the FreeBSD net. */
extern bsdnet_secure_impl_t oskit_freebsd_net_secure_impl;

#endif /* FLASK */

/*****************************************************************************/

#endif /* _OSKIT_FREEBSD_NET_BSDNET_COM_H */

/* End of file. */

