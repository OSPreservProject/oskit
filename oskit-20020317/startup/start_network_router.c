/*
 * Copyright (c) 1997-1999, 2001 University of Utah and the Flux Group.
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
 * start_network_router.c
 *
 * Start up all available network interfaces on this machine.
 */
#include <oskit/startup.h>

#ifdef	PTHREADS
#include <oskit/threads/pthread.h>

#define start_network_router	start_network_router_pthreads
#define start_conf_network	start_conf_network_pthreads
#endif

void
start_network_router()
{
	/* Set the FreeBSD network routing code to allow routing */
	start_conf_network_router();

	/* Start all available interfaces. */
	start_conf_network(-1); 
}
