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
 * declarations for our internal implementation of the oskit_socket_t
 * and oskit_opensocket_t interfaces based on the freebsd net library.
 */
#ifndef _OSKIT_NET_SOCKIMPL_H
#define _OSKIT_NET_SOCKIMPL_H

#include <oskit/io/asyncio.h>
#include <oskit/io/bufio_stream.h>
#include <oskit/com/listener_mgr.h>
#include <oskit/net/socket.h>
#if FLASK
#include <oskit/net/socket_secure.h>
#endif
struct socket;


struct oskit_sockimpl {
	oskit_socket_t		ioi;		/* COM I/O interface Nr. 1 */
	oskit_stream_t		ios;		/* COM I/O interface Nr. 2 */
	oskit_asyncio_t		ioa;		/* COM I/O interface Nr. 3 */
	oskit_bufio_stream_t	iob;		/* COM I/O interface Nr. 4 */
#if FLASK
	oskit_socket_secure_t	sosi;
#endif
	
	unsigned		count;		/* reference count */

	struct socket *		so;		/* BSD socket structure */
	struct listener_mgr *	readers;	/* lis'ners for asyncio READ */
	struct listener_mgr *	writers;	/* lis'ners for asyncio WRITE*/
};
typedef struct oskit_sockimpl oskit_sockimpl_t;

/*
 *
 */
oskit_sockimpl_t *bsdnet_create_sockimpl(struct socket *);

#endif  /* _OSKIT_NET_SOCKIMPL_H */

