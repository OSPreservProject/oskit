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
 * start_network_native.c
 *
 * start up native socket interface.
 */
#include <oskit/dev/error.h>
#include <oskit/net/socket.h>

#include <oskit/c/stdio.h>
#include <oskit/c/string.h>
#include <oskit/c/assert.h>

oskit_error_t oskit_native_net_init(oskit_socket_factory_t **f);

void
start_network_native(void)
{
	oskit_socket_factory_t *fsc;
	int   err;
 
	err = oskit_native_net_init(&fsc);
	assert(!err);
	oskit_register(&oskit_socket_factory_iid, (void *) fsc);
}

