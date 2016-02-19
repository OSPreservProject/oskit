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
 * OSKit definition of additional stream interfaces using bufios
 */
#ifndef _OSKIT_IO_BUFIO_STREAM_H_
#define _OSKIT_IO_BUFIO_STREAM_H_

#include <oskit/com.h>
struct oskit_bufio;

/*
 * A simple read/write stream I/O interface that takes bufios
 * instead of copying data. It is intended for reads/writes where
 * the writer does not use the memory after the write and where
 * the reader does not need ownership of the buffer that was read.
 *
 * IID 4aa7dfb0-7c74-11cf-b500-08000953adc2
 */
struct oskit_bufio_stream {
	struct oskit_bufio_stream_ops *ops;
};
typedef struct oskit_bufio_stream oskit_bufio_stream_t;

struct oskit_bufio_stream_ops {

	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL_IUNKNOWN(oskit_bufio_stream_t)

	/* Operations specific to the bufio_stream interface */

	/* read a bufio, bytes == 0 signals eof */
	OSKIT_COMDECL	(*read)(oskit_bufio_stream_t *s, 
				struct oskit_bufio **buf,
				oskit_size_t *bytes);

	/* write a bufio, data is starting at `offset' */
	OSKIT_COMDECL	(*write)(oskit_bufio_stream_t *s, 
				struct oskit_bufio *buf,
				oskit_size_t	offset);
};

#define oskit_bufio_stream_query(io, iid, out_ihandle) \
        ((io)->ops->query((oskit_bufio_stream_t *)(io), (iid), (out_ihandle)))
#define oskit_bufio_stream_addref(io) \
        ((io)->ops->addref((oskit_bufio_stream_t *)(io)))
#define oskit_bufio_stream_release(io) \
        ((io)->ops->release((oskit_bufio_stream_t *)(io)))
#define oskit_bufio_stream_read(s, buf, offset) \
        ((s)->ops->read((oskit_bufio_stream_t *)(s), (buf), (offset)))
#define oskit_bufio_stream_write(s, buf, offset) \
        ((s)->ops->write((oskit_bufio_stream_t *)(s), (buf), (offset)))

/* GUID for oskit_bufio_stream interface */
extern const struct oskit_guid oskit_bufio_stream_iid;
#define OSKIT_BUFIO_STREAM_IID OSKIT_GUID(0x4aa7dfb0, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif /* _OSKIT_IO_BUFIO_STREAM_H_ */
