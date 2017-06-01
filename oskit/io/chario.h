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
 * Definition of the symmetric character-based I/O interface
 * used to pass packets between character drivers and protocols.
 * XXX Note: This interface definition isn't complete or even functional yet.
 */
#ifndef _OSKIT_IO_CHARIO_H_
#define _OSKIT_IO_CHARIO_H_

#include <oskit/com.h>

/*
 * Simple symmetric character-based I/O interface,
 * IID 4aa7dfa0-7c74-11cf-b500-08000953adc2.
 *
 * This interface only provides sender-driven, push-style data transfer;
 * to implement two-way asynchronous data transfer,
 * the two communicating parties must each implement a chario interface
 * and provide a reference to their own chario interface to the other.
 * Thus, the chario interface essentially represents a "character consumer".
 */
struct oskit_chario {
	struct oskit_chario_ops *ops;
};
typedef struct oskit_chario oskit_chario_t;

struct oskit_chario_ops {
	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_chario_t *dev,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_chario_t *dev);
	OSKIT_COMDECL_U	(*release)(oskit_chario_t *dev);

	/*
	 * Push one or more characters through to the consumer.
	 * The data is provided in a simple local memory buffer.
	 */
	OSKIT_COMDECL 	(*push)(oskit_chario_t *io,
				const void *buf, oskit_size_t size);

	/*
	 * XXX throttle, unthrottle, start, stop, termios, etc. ???
	 */
};

extern const struct oskit_guid oskit_chario_iid;
#define OSKIT_CHARIO_IID OSKIT_GUID(0x4aa7dfa0, 0x7c74, 0x11cf, 		\
                0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_chario_query(dev, iid, out_ihandle) \
	((dev)->ops->query((oskit_chario_t *)(dev), (iid), (out_ihandle)))
#define oskit_chario_addref(dev) \
	((dev)->ops->addref((oskit_chario_t *)(dev)))
#define oskit_chario_release(dev) \
	((dev)->ops->release((oskit_chario_t *)(dev)))
#define oskit_chario_push(io, buf, size) \
	((io)->ops->push((oskit_chario_t *)(io), (buf), (size)))

/*
 * This function provides a default chario object implementation,
 * which simply passes incoming packets on to a specified callback function.
 * This implementation is only a convenience; its use is not mandatory.
 * Returns NULL if there is not enough memory to create the chario object.
 */
oskit_chario_t *oskit_chario_create(oskit_error_t (*func)(void *data,
						       const void *buf,
						       oskit_size_t size),
				  void *data);

#endif /* _OSKIT_IO_CHARIO_H_ */
