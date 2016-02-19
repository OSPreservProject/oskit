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
 * Extension of standard netio interface - allows to register multiple
 * listeners which all receive copies of pushed packets
 */
#ifndef _OSKIT_IO_NETIO_FANOUT_H_
#define _OSKIT_IO_NETIO_FANOUT_H_

#include <oskit/com.h>
#include <oskit/io/bufio.h>
#include <oskit/io/netio.h>

/*
 * Simple extension to the network I/O interface allowing packet filters
 * IID 4aa7dfa2-7c74-11cf-b500-08000953adc2
 */
struct oskit_netio_fanout {
	struct oskit_netio_fanout_ops *ops;
};
typedef struct oskit_netio_fanout oskit_netio_fanout_t;

struct oskit_netio_fanout_ops {
	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_netio_fanout_t *dev, 
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_netio_fanout_t *dev);
	OSKIT_COMDECL_U	(*release)(oskit_netio_fanout_t *dev);

	/*
	 * Push a packet through to the packet consumer.
	 * If the consumer needs to hold on to the provided bufio object
	 * after the call returns, it will call b->addref() to obtain a ref,
	 * then release it sometime later when it is done with the buffer.
	 * Otherwise, if the consumer doesn't get its own bufio reference,
	 * the caller may recycle the buffer as soon as the call returns.
	 * The passed buffer object is logically read-only;
	 * the consumer must not attempt to write to it.
	 * The size parameter to this call is the actual size of the packet;
	 * the size of the buffer, as returned by b->getsize(),
	 * may be larger than the size of the packet.
	 */
	OSKIT_COMDECL 	(*push)(oskit_netio_fanout_t *io,
				oskit_bufio_t *b,
				oskit_size_t size);

	/*
	 * add a listener to be included in the fan out
	 */
	OSKIT_COMDECL	(*add_listener)(oskit_netio_fanout_t *io,
				oskit_netio_t *c);

	/*
	 * exclude a listener from the fan out
	 */
	OSKIT_COMDECL	(*remove_listener)(oskit_netio_fanout_t *io,
				oskit_netio_t *c);
};

extern const struct oskit_guid oskit_netio_fanout_iid;
#define OSKIT_NETIO_FANOUT_IID OSKIT_GUID(0x4aa7dfa2, 0x7c74, 0x11cf, 	\
                0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_netio_fanout_query(dev, iid, out_ihandle) \
	((dev)->ops->query((oskit_netio_fanout_t *)(dev), (iid), (out_ihandle)))
#define oskit_netio_fanout_addref(dev) \
	((dev)->ops->addref((oskit_netio_fanout_t *)(dev)))
#define oskit_netio_fanout_release(dev) \
	((dev)->ops->release((oskit_netio_fanout_t *)(dev)))
#define oskit_netio_fanout_push(io, b, size) \
	((io)->ops->push((oskit_netio_fanout_t *)(io), (b), (size)))
#define oskit_netio_fanout_add_listener(io, b) \
	((io)->ops->add_listener((oskit_netio_fanout_t *)(io), (b)))
#define oskit_netio_fanout_remove_listener(io, b) \
	((io)->ops->remove_listener((oskit_netio_fanout_t *)(io), (b)))

/*
 * create an fanout netio instance
 */
oskit_netio_fanout_t *oskit_netio_fanout_create();

#endif /* _OSKIT_IO_NETIO_FANOUT_H_ */
