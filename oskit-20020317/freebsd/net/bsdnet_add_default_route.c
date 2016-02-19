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
 * This file contains a function to set up a default route - 
 * and no, I don't mean this seriously.
 *
 * It is of temporary nature.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include "net/if.h"
#include "net/route.h"
#include "netinet/in.h"
#include "netinet/if_ether.h"
#include "netinet/in_var.h"
#include <arpa/inet.h>
#include <oskit/net/freebsd.h>

#include "glue.h"

/*
 * you might ask: where does this array come from?
 * well, it's just a recording of what route writes to its socket
 * when invoked as 'route add default 155.99.214.61'
 * we take this bunch of data and shove it into a routing socket
 *
 * an ambitious person might try to decipher 'man 4 route' or the 
 * code in /usr/src/sys/route/route.c
 */
#define ARRAYSIZE 128

static char array[] = {
 0x80, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0x08, 0x00, 0x00,
 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02, 0x00, 0x00,

 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x10, 0x02, 0x00, 0x00, 0x9B, 0x63, 0xD6, 0x3D, 0x00, 0x00, 0x00, 0x00,
/*********************** ^^^^^^^^^^^^^^^^^^^^^^ that's ADOFFSET *********/
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};
#define ADOFFSET (12*8+12+4)

static int 
_oskit_freebsd_net_add_default_route(char *gateway)
{
	int error;
	struct socket *so;
	struct proc p;

	OSKIT_FREEBSD_CREATE_CURPROC(p);

	/* set gateway */
	inet_aton(gateway, (struct in_addr *)(array + ADOFFSET));

	error = socreate(AF_ROUTE, &so, SOCK_RAW, 0, &p);
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
	if (error) 
		return error;

	if ((error = bsdnet_write(so, array, ARRAYSIZE, 0)))
		return error;

	OSKIT_FREEBSD_CREATE_CURPROC(p);
	error = soclose(so);
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
	return error;
}

oskit_error_t 
oskit_freebsd_net_add_default_route(char *gateway)
{
	return /* XXX: errno_to_oskit_errno() */ 
		_oskit_freebsd_net_add_default_route(gateway);
}
