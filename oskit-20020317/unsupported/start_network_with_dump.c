/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * tcpdump.c
 * 
 * this is not a real tcpdump, but simply something that inserts itself
 * between the TCP/IP stack and the networking driver
 */
#include <oskit/dev/error.h>
#include <oskit/dev/ethernet.h>
#include <oskit/dev/freebsd.h>
#include <oskit/dev/net.h>
#include <oskit/net/freebsd.h>
#include <oskit/net/socket.h>
#include <fd.h>
#include <oskit/startup.h>
#include <oskit/clientos.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* XXX move in header file */
oskit_netio_t * oskit_netio_link_create(
	void (*dump_func)(void *cookie, void *p, oskit_size_t len),
	void *cookie,
	oskit_netio_t *forwardto);

static void (*the_dump_func)(void *,void*,oskit_size_t) = NULL;

/*
 * Invoke this in place of libstartup's start_network().
 *
 * XXX Note that this code has not been tested after start_network()'s
 * internals were changed.  This *should* work, but hey that's never
 * happened in my lifetime...
 */
void
start_network_with_dump(void (*dump_func)(void *cookie, void *p, unsigned len))
{
	the_dump_func = dump_func;

	start_network();
}

/*
 * XXX override the ...open_eif() command to do our customizations.
 */
int start_conf_network_open_eif(oskit_etherdev_t *etherdev,
				oskit_freebsd_net_ether_if_t **eif)
{
	oskit_netio_t	*send_nio;
	oskit_netio_t   *recv_nio;
	int err;

	/* this sets eif.recv_nio */
	err = oskit_freebsd_net_prepare_ether_if(eif);
	if (err)
		return err;
	

	/* we must set eif->haddr and eif->send_nio */
	oskit_etherdev_getaddr(etherdev,
			       (*eif)->haddr);


        /* Open this adaptor, exchanging net_io devices */
	recv_nio = oskit_netio_link_create(the_dump_func, 
		(void *)0 /* incoming */,
		(*eif)->recv_nio);
	oskit_netio_release((*eif)->recv_nio);

        err = oskit_etherdev_open(etherdev,
				  0,
				  recv_nio,
				  &send_nio);
        if (err) {
                // osenv_log(OSENV_LOG_ERR, "Error opening ethercard");
                return err;
        }

	(*eif)->send_nio = oskit_netio_link_create(the_dump_func, 
		(void *)1 /* outgoing */,
		send_nio);
	oskit_netio_release(send_nio);

	return 0;
}
