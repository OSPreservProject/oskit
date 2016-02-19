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
 * The osenv_safe interface is implemented by the threads module. It is
 * intended to provide components with thread-safe adaptors of various
 * interfaces. For example, a component wishing to convert a socket
 * interface into a thread-safe socket interface would use the socket
 * method. The new interface can safely be used in a multi-threaded
 * environment, with the necessary process locking done in the adaptor.
 */

#ifndef _OSKIT_DEV_OSENV_SAFE_H_
#define _OSKIT_DEV_OSENV_SAFE_H_

#include <oskit/com.h>

struct oskit_stream;
struct oskit_asyncio;
struct oskit_socket;

/*
 * Interface used to query the module implementing osenv_ for adaptors.
 * IID 4aa7dfb3-7c74-11cf-b500-08000953adc2.
 */
struct osenv_safe {
        struct osenv_safe_ops *ops;
};
typedef struct osenv_safe osenv_safe_t;

struct osenv_safe_ops {

        /* COM-specified IUnknown interface operations */
        OSKIT_COMDECL_IUNKNOWN(osenv_safe_t);

        /*** Operations specific to osenv_safe_t ***/

        /*
         * Various routines to get safe interfaces.
         */
        OSKIT_COMDECL   (*stream)(osenv_safe_t *s,
                                   struct oskit_stream *unosenv_safe_stream, 
				   struct oskit_stream **osenv_safe_stream);

        OSKIT_COMDECL   (*asyncio)(osenv_safe_t *s,
                                   struct oskit_asyncio *unosenv_safe_asyncio, 
				   struct oskit_asyncio **osenv_safe_asyncio);

        OSKIT_COMDECL   (*socket)(osenv_safe_t *s,
                                   struct oskit_socket *unosenv_safe_socket, 
				   struct oskit_socket **osenv_safe_socket);

	/*
	 * Wrap a combination of socket/stream/asyncio/bufio_stream interfaces,
	 * given as a socket interface.
	 */
        OSKIT_COMDECL   (*sockio)(osenv_safe_t *s,
                                   struct oskit_socket *unosenv_safe_socket, 
				   struct oskit_socket **osenv_safe_socket);

	/* add more interfaces here.
	 * candidates are oskit_file, oskit_dir, etc.
	 */
};

/* GUID for pthread_interfaces interface */
extern const struct oskit_guid osenv_safe_iid;
#define OSENV_SAFE_IID OSKIT_GUID(0x4aa7dfb3, 0x7c74, 0x11cf, \
                                0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define osenv_safe_query(s, iid, out_ihandle) \
        ((s)->ops->query((osenv_safe_t *)(s), (iid), (out_ihandle)))
#define osenv_safe_addref(s) \
        ((s)->ops->addref((osenv_safe_t *)(s)))
#define osenv_safe_release(s) \
        ((s)->ops->release((osenv_safe_t *)(s)))
#define osenv_safe_stream(s, in_interface, out_interface) \
        ((s)->ops->stream((osenv_safe_t *)(s), (in_interface), (out_interface)))
#define osenv_safe_asyncio(s, in, out) \
        ((s)->ops->asyncio((osenv_safe_t *)(s), (in), (out)))
#define osenv_safe_socket(s, in_interface, out_interface) \
        ((s)->ops->socket((osenv_safe_t *)(s), (in_interface), (out_interface)))
#define osenv_safe_sockio(s, in_interface, out_interface) \
        ((s)->ops->sockio((osenv_safe_t *)(s), (in_interface), (out_interface)))

#endif /* _OSKIT_DEV_OSENV_SAFE_H_ */

