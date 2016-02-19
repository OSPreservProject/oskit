/*
 * Copyright (c) 1997-1998, 2000 University of Utah and the Flux Group.
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
 * Definition of the symmetric network I/O interface
 * used to pass packets between network drivers and protocols.
 */
#ifndef _OSKIT_IO_NETIO_H_
#define _OSKIT_IO_NETIO_H_

#include <oskit/com.h>
#include <oskit/io/bufio.h>

/*
 * Simple symmetric network I/O interface,
 * IID 4aa7df8b-7c74-11cf-b500-08000953adc2.
 *
 * The only operation that this interface provides is push, which 
 * allows the sender to transfer a packet to the reciever.  The
 * protocol is symmetric in that on open you must supply a netio
 * object so that packets can be sent back.
 * Thus, the netio interface essentially represents a "packet consumer".
 */
struct oskit_netio {
	struct oskit_netio_ops *ops;
};
typedef struct oskit_netio oskit_netio_t;

struct oskit_netio_ops {
	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_netio_t *dev, const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_netio_t *dev);
	OSKIT_COMDECL_U	(*release)(oskit_netio_t *dev);

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
	OSKIT_COMDECL 	(*push)(oskit_netio_t *io,
				oskit_bufio_t *b,
				oskit_size_t size);

	/*
	 * Allow the provider of the netio interface to supply a bufio
	 * allocator to the user of the netio. The allocator should
	 * endeavor to provide an implementation of oskit_bufio_t that is
	 * most efficient for the provider. Returns OSKIT_NOTIMPL if
	 * netio implementation does not provide this method, in which case
	 * the caller needs to fall back and allocated a bufio by some
	 * other method. 
	 */
	OSKIT_COMDECL 	(*alloc_bufio)(oskit_netio_t *io,
				oskit_size_t size,
				oskit_bufio_t **out_bufio);
};

extern const struct oskit_guid oskit_netio_iid;
#define OSKIT_NETIO_IID OSKIT_GUID(0x4aa7df8b, 0x7c74, 0x11cf, 		\
                0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)


#define oskit_netio_query(dev, iid, out_ihandle) \
	((dev)->ops->query((oskit_netio_t *)(dev), (iid), (out_ihandle)))
#define oskit_netio_addref(dev) \
	((dev)->ops->addref((oskit_netio_t *)(dev)))
#define oskit_netio_release(dev) \
	((dev)->ops->release((oskit_netio_t *)(dev)))
#define oskit_netio_push(io, b, size) \
	((io)->ops->push((oskit_netio_t *)(io), (b), (size)))
#define oskit_netio_alloc_bufio(io, size, out_bufio) \
	((io)->ops->alloc_bufio((oskit_netio_t *)(io), (size), (out_bufio)))

/*
 * This function provides a default netio object implementation,
 * which simply passes incoming packets on to a specified callback function.
 * This implementation is only a convenience; its use is not mandatory.
 * Returns NULL if there is not enough memory to create the netio object.
 */
oskit_netio_t *oskit_netio_create(oskit_error_t (*func)(void *data, 
						     oskit_bufio_t *b,
						     oskit_size_t pkt_size),
				void *data);

/*
 * This creates a netio object that calls a cleanup function when
 * the last reference is released.  (Note that oskit_netio_create is
 * just a wrapper for this function).
 */
oskit_netio_t * oskit_netio_create_cleanup(oskit_error_t (*func)(void *data,
						oskit_bufio_t *b,
                                   		oskit_size_t pkt_size),
              		void *data, void (*destructor)(void *data));

#endif /* _OSKIT_IO_NETIO_H_ */
