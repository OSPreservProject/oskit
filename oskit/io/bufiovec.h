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
 * Definition of an scatter/gather interface
 */
#ifndef _OSKIT_IO_BUFIOVEC_H_
#define _OSKIT_IO_BUFIOVEC_H_

#include <oskit/types.h>
#include <oskit/com.h>
#include <oskit/io/iovec.h>

/*
 * Basic scatter/gather I/O interface,
 * IID 4aa7dfb2-7c74-11cf-b500-08000953adc2.
 *
 * Objects exporting the extended buffer (oskit_bufio) interface may also
 * support this interface, allowing a caller to map the data even if it is
 * in an array of scatter/gather buffers.
 */
struct oskit_bufiovec {
	struct oskit_bufiovec_ops *ops;
};
typedef struct oskit_bufiovec oskit_bufiovec_t;

struct oskit_bufiovec_ops {
	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL	(*query)(oskit_bufiovec_t *dev,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_bufiovec_t *dev);
	OSKIT_COMDECL_U	(*release)(oskit_bufiovec_t *dev);

	/*
	 * Return an array of iovecs that represents a scatter/gather buffer
	 * Otherwise, all the comments on bufio::map apply.
	 */
	OSKIT_COMDECL_U	(*map)(oskit_bufiovec_t *io, 
			       oskit_iovec_t *vec, oskit_u32_t veclen);

	/* more later... unmap */
};


extern const struct oskit_guid oskit_bufiovec_iid;
#define OSKIT_BUFIOVEC_IID OSKIT_GUID(0x4aa7dfb2, 0x7c74, 0x11cf, \
                0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_bufiovec_query(dev, iid, out_ihandle) \
	((dev)->ops->query((oskit_bufiovec_t *)(dev), (iid), (out_ihandle)))
#define oskit_bufiovec_addref(dev) \
	((dev)->ops->addref((oskit_bufiovec_t *)(dev)))
#define oskit_bufiovec_release(dev) \
	((dev)->ops->release((oskit_bufiovec_t *)(dev)))
#define oskit_bufiovec_map(io, vec, veclen) \
	((io)->ops->map((oskit_bufiovec_t *)(io), (vec), (veclen)))

/* no default implementation yet */

#endif /* _OSKIT_IO_BUFIOVEC_H_ */
