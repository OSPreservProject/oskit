/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Definitions specific to fdev object adapters
 */
#ifndef _OSKIT_DEV_SOA_H_
#define _OSKIT_DEV_SOA_H_

#include <oskit/io/netio.h>
#include <oskit/fs/openfile.h>
#include <oskit/com/listener.h>

/*
 * Create a `oskit_openfile' adapter wrapped around a pair of netio's
 * with a bounded queue of length `queuelength'.
 *
 * `exchange' will be called with `exchange_arg' and the netio object
 * on which incoming packets are to be pushed; it should return a netio
 * object on which outgoing packets can be pushed; or NULL on failure.
 *
 * The created openfile object will return `file' when getfile is called.
 *
 * `flags' is a standard oskit_file_open flags arg.
 *
 * It will notify `notify_on_dump' if a packet is being dumped.
 * Note that the packet at the time of the notification is already
 * dequeued, and will be released immediately after the notify.
 * The iunknown passed to notify is the oskit_bufio of the packet
 */
OSKIT_COMDECL
create_openfile_from_netio(
        oskit_netio_t *(*exchange)(void *, oskit_netio_t *f),
                void *exchange_arg,
                int queuelength,
                oskit_oflags_t flags,
		oskit_listener_t	*notify_on_dump,
                struct oskit_file       *file,
                struct oskit_openfile **out_openfile);

/*
 * Create an `oskit_listener_t' object from an `oskit_sleep_t' object.
 * The notify method of the listener will call the wakeup method of the
 * sleep object.
 */
struct oskit_sleep;
oskit_listener_t *oskit_sleep_create_listener(struct oskit_sleep *sleeper);


/*
 * Create a stream/asyncio object based around an input character queue
 * of the given size.  The given stream is used for output written to
 * the created stream.  The returned upcall stream should be written to
 * at interrupt level to deliver characters to the input queue; its write
 * method returns OSKIT_EWOULDBLOCK when the queue is full.
 */
struct oskit_osenv_intr;
struct oskit_osenv_sleep;
OSKIT_COMDECL
oskit_charqueue_intr_stream_create(oskit_size_t size,
				   oskit_stream_t *stream_for_output,
				   struct oskit_osenv_intr *intr,
				   struct oskit_osenv_sleep *sleep,
				   oskit_stream_t **out_stream,
				   oskit_stream_t **out_upcall_stream);


#endif /* _OSKIT_DEV_SOA_H_ */
