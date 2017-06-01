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
 * This is a simply netio object which works a link between two netio
 * object, calling a function on every push. Can be used for packet 
 * monitoring/counting.
 */

#include <oskit/io/netio.h>
#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/malloc.h>
#include <oskit/c/assert.h>

struct netio_link {
	oskit_netio_t ioi;		/* COM interface */
	unsigned count;			/* reference count */
	oskit_netio_t   *forwardto;
	void            *cookie;
	void (*dump_f)(void *, void *, oskit_size_t);
};

/*
 * Query a net I/O object for its interfaces.
 */
static OSKIT_COMDECL
net_query(oskit_netio_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
	struct netio_link *po = (struct netio_link *)io;

	assert (po != NULL);
	assert (po->count != 0);

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_netio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &po->ioi;
		++po->count;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}


/*
 * Clone a reference to a netio object
 */
static OSKIT_COMDECL_U
net_addref(oskit_netio_t *io)
{
	struct netio_link *po = (struct netio_link *)io;

	assert (po != NULL);
	assert (po->count != 0);

	return ++po->count;
}


/*
 * Close ("release") a netio object.
 */
static OSKIT_COMDECL_U
net_release(oskit_netio_t *io)
{
	struct netio_link *nio = (struct netio_link *)io;
	unsigned newcount;

	assert (nio != NULL);
	assert (nio->count != 0);

	newcount = nio->count - 1;
	if (newcount == 0) {
		oskit_netio_release(nio->forwardto);
		free(nio);
		return 0;
	}

	return nio->count = newcount;
}

/*
 * This is the function that does the filtering.
 */
static OSKIT_COMDECL
net_push(oskit_netio_t *ioi, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	struct netio_link *nio = (struct netio_link *)ioi;
	void *p;
	oskit_error_t maperr, rc;
	oskit_size_t len = pkt_size;

	assert (nio != NULL);
	assert (nio->count != 0);

        maperr = oskit_bufio_map(b, &p, 0, pkt_size);
	if (maperr) {
		static unsigned char buf[2048];
		rc = oskit_bufio_read(b, buf, 0, pkt_size, &len);
		p = buf;
		assert(rc == 0);
	}
	nio->dump_f(nio->cookie, p, len);
	if (!maperr)
		oskit_bufio_unmap(b, p, 0, pkt_size);

	return oskit_netio_push(nio->forwardto, b, pkt_size);
}

/*
 * Unimplemented bufio allocator
 */
static OSKIT_COMDECL
net_alloc_bufio(oskit_netio_t *io, oskit_size_t size,
		oskit_bufio_t **out_bufio)
{
	return OSKIT_E_NOTIMPL;
}

/*
 * netio_link vtable
 */
static 
struct oskit_netio_ops netio_ops = {
	net_query,
	net_addref, 
	net_release,
	net_push,
	net_alloc_bufio
};

/*
 * This function creates a netio link.
 */
oskit_netio_t *
oskit_netio_link_create(
	void (*dump_f)(void *cookie, void *p, oskit_size_t len),
		void *cookie,
		oskit_netio_t *forwardto)
{
	struct netio_link *nio;

	nio = malloc(sizeof(*nio));
	if (!nio)
		return 0;

	nio->forwardto = forwardto;
	nio->dump_f = dump_f;
	nio->cookie = cookie;

	oskit_netio_addref(nio->forwardto);
	nio->ioi.ops = &netio_ops;
	nio->count = 1;
	return &nio->ioi;
}
