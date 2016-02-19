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
 * Definition of a COM interface to support basic asynchronous I/O,
 * based on registered callback objects.
 * This can be used, for example,
 * to implement Unix SIGIO or select() functionality,
 * or POSIX.1b aio functionality.
 */
#ifndef _OSKIT_IO_ASYNCIO_H_
#define _OSKIT_IO_ASYNCIO_H_

#include <oskit/com.h>
#include <oskit/com/listener.h>

/*
 * This interface supports a notion
 * of three basic kinds of interesting events:
 * readability, writeability, and "other" exceptional conditions -
 * basically corresponding to the select()'s three fd sets.
 */
#define OSKIT_ASYNCIO_READABLE	1
#define OSKIT_ASYNCIO_WRITABLE	2
#define OSKIT_ASYNCIO_EXCEPTION	4
#define OSKIT_ASYNCIO_ALL	(OSKIT_ASYNCIO_READABLE | \
				 OSKIT_ASYNCIO_WRITABLE | \
				 OSKIT_ASYNCIO_EXCEPTION)

/*
 * Asynchronous I/O interface,
 * IID 4aa7dfa7-7c74-11cf-b500-08000953adc2.
 * I/O objects such as streams can export this interface
 * in order to provide support for asynchronous I/O.
 */
struct oskit_asyncio {
	struct oskit_asyncio_ops *ops;
};
typedef struct oskit_asyncio oskit_asyncio_t;

struct oskit_asyncio_ops {

	/*** Operations inherited from IUnknown interface ***/
	OSKIT_COMDECL	(*query)(oskit_asyncio_t *io,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_asyncio_t *io);
	OSKIT_COMDECL_U	(*release)(oskit_asyncio_t *io);

	/*** Operations specific to the asyncio interface ***/

	/*
	 * Poll for currently pending asynchronous I/O conditions.
	 * If successful, returns a mask of the OSKIT_ASYNC_IO_* flags above,
	 * indicating which conditions are currently present.
	 */
	OSKIT_COMDECL	(*poll)(oskit_asyncio_t *io);

	/*
	 * Add a callback object (a "listener" for async I/O events).
	 * When an event of interest occurs on this I/O object
	 * (i.e., when one of the one to three I/O conditions becomes true),
	 * *all* registered listeners will be called.
	 *
	 * mask is an ORed combination of OSKIT_ASYNCIO_* flags.
	 * It specifies which events the listener is interested in.
	 * Note that spurious notifications are possible - the listener
	 * must call poll to determine the actual state of affairs.
	 *
	 * Also, if successful, this method returns a mask
	 * describing which of the OSKIT_ASYNC_IO_* conditions are already true,
	 * which the caller must check in order to avoid missing events
	 * that occur just before the listener is registered.
	 */
	OSKIT_COMDECL	(*add_listener)(oskit_asyncio_t *io,
					struct oskit_listener *l,
					oskit_s32_t mask);

	/*
	 * Remove a previously registered listener callback object.
	 * Returns an error if the specified callback has not been registered.
	 */
	OSKIT_COMDECL	(*remove_listener)(oskit_asyncio_t *io,
					   struct oskit_listener *l);

	/*
	 * Returns the number of bytes that can be read. 
	 */
	OSKIT_COMDECL	(*readable)(oskit_asyncio_t *io);
};

/* GUID for oskit_asyncio interface */
extern const struct oskit_guid oskit_asyncio_iid;
#define OSKIT_ASYNCIO_IID OSKIT_GUID(0x4aa7dfa7, 0x7c74, 0x11cf, \
			0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_asyncio_query(io, iid, out_ihandle) \
	((io)->ops->query((oskit_asyncio_t *)(io), (iid), (out_ihandle)))
#define oskit_asyncio_addref(io) \
	((io)->ops->addref((oskit_asyncio_t *)(io)))
#define oskit_asyncio_release(io) \
	((io)->ops->release((oskit_asyncio_t *)(io)))
#define oskit_asyncio_poll(io) \
	((io)->ops->poll((oskit_asyncio_t *)(io)))
#define oskit_asyncio_add_listener(io, l, mask) \
	((io)->ops->add_listener((oskit_asyncio_t *)(io), (l), (mask)))
#define oskit_asyncio_remove_listener(io, l) \
	((io)->ops->remove_listener((oskit_asyncio_t *)(io), (l)))
#define oskit_asyncio_readable(io) \
	((io)->ops->readable((oskit_asyncio_t *)(io)))

#endif /* _OSKIT_IO_ASYNCIO_H_ */
