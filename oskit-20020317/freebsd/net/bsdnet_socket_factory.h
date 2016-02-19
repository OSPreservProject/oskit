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
 * Declarations for the FreeBSD networking stack's implementation of the
 * `oskit_socket_factory' COM interface.
 */
#ifndef _OSKIT_FREEBSD_NET_BSDNET_SOCKET_FACTORY_H
#define _OSKIT_FREEBSD_NET_BSDNET_SOCKET_FACTORY_H

#include <oskit/net/socket.h>

oskit_error_t
bsdnet_socket_factory_create_helper(
	oskit_u32_t domain,
	oskit_u32_t type,
	oskit_u32_t protocol, 
#if FLASK
	oskit_security_id_t sid,
#endif
	oskit_socket_t **aso);

OSKIT_COMDECL
bsdnet_socket_factory_create(
	oskit_socket_factory_t *factory,
	oskit_u32_t domain,
	oskit_u32_t type,
	oskit_u32_t protocol,
	oskit_socket_t **aso);

OSKIT_COMDECL
bsdnet_socket_factory_create_pair(
	oskit_socket_factory_t *factory,
	oskit_u32_t domain,
	oskit_u32_t type,
	oskit_u32_t protocol,
	oskit_socket_t **aso1,
	oskit_socket_t **aso2);

#endif /* _OSKIT_FREEBSD_NET_BSDNET_SOCKET_FACTORY_H */

/* End of file. */

