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

/* 
 * declarations for some functions needed by the glue code
 * the name of this file is somewhat misleading.
 */
#ifndef _BSDNET_NET_IO_H
#define _BSDNET_NET_IO_H

#include <oskit/types.h>

/* opaque structures */
struct ifnet;
struct oskit_bufio;
struct oskit_socket;

/* call back function used to receive packets */
oskit_error_t bsdnet_net_receive(void *data, struct oskit_bufio *b, 
	oskit_size_t pkt_size);

/* generic driver ioctl function */
int bsdnet_driver_ioctl(struct ifnet *, int, char *);

/* generic driver start */
int bsdnet_driver_start(struct ifnet *);

/* array of software interrupts */
extern	void    (*netisrs[32]) (void);

#endif /* _BSDNET_NET_IO_H */

