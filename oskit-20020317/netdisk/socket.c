/*
 * Copyright (c) 1997-2001 University of Utah and the Flux Group.
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>

#include <oskit/net/socket.h>
#include <oskit/com/services.h>

#include "timer.h"

oskit_error_t
comsocket(int domain, int type, int protocol, oskit_socket_t **out_sock)
{
	static oskit_socket_factory_t	*fsc;
	oskit_socket_t			*sock;
	oskit_error_t			err;

	if (fsc == 0) {
		/* note that lookup_first always returns null */
		oskit_lookup_first(&oskit_socket_factory_iid, (void *) &fsc);
	}

	if (fsc == 0) {
		/*
		 * If we don't have a socket factory,
		 * we just don't support any protocols...
		 */
		return OSKIT_ENOPROTOOPT;
	}

	/* Create the socket */
	err= oskit_socket_factory_create(fsc, domain, type, protocol, &sock);
	if (err)
		return err;

#ifdef  FREEBSD_NET
	/*
	 * To be compatible with UDP lib, make the RCV timeout zero.
	 */
	{
		struct	timeval tv;

		tv.tv_sec  = 0;
		/*
		 * XXX Must be greater than a single tick or the freebsd
		 * code treats it as zero, which means block forever.
		 */
		tv.tv_usec = 1000000 / TIMER_FREQ;
		err = oskit_socket_setsockopt(sock, OSKIT_SOL_SOCKET,
					      OSKIT_SO_RCVTIMEO,
					      &tv, sizeof(tv));
		if (err)
			return err;
	}
#endif

	*out_sock = sock;
	return 0;
}

