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

#include <oskit/net/freebsd.h>

extern oskit_error_t oskit_freebsd_net_init2(oskit_socket_factory_t **outfact);
extern char ipaddr[20];
extern char gateway[20];
extern char netmask[20];
extern struct oskit_freebsd_net_ether_if *ether_if;

oskit_socket_factory_t *oskit_socket_factory = 0;

// hackety hack hack
#define ifname "wibble"

oskit_error_t
init(void)
{
        oskit_error_t err;
 
	/* initialise and create the socket */
        err = oskit_freebsd_net_init2(&oskit_socket_factory);
        if (err)
                return err;

	/* configure the interface */
	err = oskit_freebsd_net_ifconfig(ether_if, ifname, ipaddr, netmask);
	if (err)
                /* ToDo: should release the factory if we die */
                return err;

	err = oskit_freebsd_net_add_default_route(gateway);
        return err;
}


oskit_error_t
fini(void)
{
        if (oskit_socket_factory) {
                oskit_socket_factory_release(oskit_socket_factory);
                oskit_socket_factory = 0;
        }
        return 0;
}
