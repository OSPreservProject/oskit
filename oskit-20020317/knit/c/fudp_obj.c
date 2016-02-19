/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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

#include <oskit/fudp.h>
#include <oskit/io/netio.h>
#include <oskit/c/arpa/inet.h>

extern char*                   ip_address;
extern unsigned                port;
extern oskit_netio_t          *send_nio;

oskit_socket_t *socket = 0;

oskit_error_t
init(void)
{
	oskit_error_t err;
	oskit_socket_factory_t *sfact;
	struct sockaddr_in sin;

        /* get a socket factory */
	err = fudp_init(&sfact);
	if (err) return err;

        /* use factory to get a socket */
	err = oskit_socket_factory_create(sfact, OSKIT_AF_INET,
					  OSKIT_SOCK_DGRAM, IPPROTO_UDP,
					  &socket);
	if (err) return err;

        /* discard factory */
        oskit_socket_factory_release(sfact);

        /* bind socket to ip_address (used for outgoing packets in fudp) */
	sin.sin_len = htons(sizeof sin);
	sin.sin_family = OSKIT_AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_address);
	sin.sin_port = htons(port);

	err = oskit_socket_bind(socket, (struct oskit_sockaddr *)&sin,
				sizeof sin);
	if (err) return err;

        /* 
         * Set the send netio object for this socket.
         *
         * fudp sockets are the only sockets that support this option:
         * it sets the netio object to be used when sending.
         * As far as I can tell, the socket is useless until you
         * set this option.
         */
	err = oskit_socket_setsockopt(socket, OSKIT_SOL_SOCKET, OSKIT_SO_NETIO,
				      send_nio, 0);
        return err;
}

oskit_error_t
fini(void)
{
    if (socket) {
            oskit_socket_release(socket);
    }
    return 0;
}
