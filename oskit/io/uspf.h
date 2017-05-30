/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * This file contains the prototype definitions for the Unsafe Packet
 * Filter NetIO consumer/producer.
 */
#ifndef _OSKIT_IO_USPF_H_
#define _OSKIT_IO_USPF_H_

#include <oskit/io/netio.h>
#include <oskit/types.h>

/*
 * create an netio which calls `func' on every packet pushed onto it
 * if func returns 1, it will push it on to `push_on' 
 */
oskit_netio_t *
oskit_netio_uspf_create(int (*func)(void *data, oskit_size_t pkt_size),
        oskit_netio_t *push_on);

#endif /* _OSKIT_IO_USPF_H_ */
