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

#include <oskit/dev/ethernet.h>
#include <oskit/dev/dev.h>
#include <oskit/io/netio.h>

extern oskit_etherdev_t *etherdev;
extern oskit_netio_t *recv_nio;
extern unsigned flags;

oskit_netio_t *send_nio = 0;

oskit_error_t
init(void)
{
	return oskit_etherdev_open(etherdev, flags, recv_nio, &send_nio);
}


oskit_error_t
fini(void)
{
        if (send_nio) {
                oskit_netio_release(send_nio);
                send_nio = 0;
        }
        return 0;
}
