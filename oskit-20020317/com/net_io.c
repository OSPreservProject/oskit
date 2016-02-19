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
 * Default netio object implementation.
 * Besides providing a (slightly) useful service,
 * this module serves as an example of how to implement a netio object.
 */

#include <oskit/io/netio.h>
#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/malloc.h>
#include <oskit/c/assert.h>

struct client_netio {
	oskit_netio_t	ioi;		/* COM I/O Interface */
	unsigned 	count;		/* reference count */
	oskit_error_t	(*func)(void *data, oskit_bufio_t *b, oskit_size_t size);
	void		*data;
	void		(*cleanup)(void *data); /* cleanup destructor */
};
typedef struct client_netio client_netio_t;

/*
 * Query a net I/O object for its interfaces.
 * This is extremely easy because we only export one interface
 * (plus its base type, IUnknown).
 */
static OSKIT_COMDECL
net_query(oskit_netio_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
	struct client_netio *po = (struct client_netio *)io;

	assert(po != NULL);
	assert(po->count != 0);

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
	struct client_netio *po = (struct client_netio *)io;

	assert(po != NULL);
	assert(po->count != 0);

	return ++po->count;
}


/*
 * Close ("release") a netio object.
 */
static OSKIT_COMDECL_U
net_release(oskit_netio_t *io)
{
	struct client_netio *po = (struct client_netio *)io;
	unsigned newcount;

	assert(po != NULL);
	assert(po->count != 0);

	newcount = po->count - 1;
	if (newcount == 0) {
		if (po->cleanup)
			po->cleanup(po->data);
		free(po);
		return 0;
	}

	return po->count = newcount;
}


/*
 * Receive a packet and forward it on to the callback function.
 */
static OSKIT_COMDECL
net_push(oskit_netio_t *ioi, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	struct client_netio *po = (struct client_netio *)ioi;

	assert(po != NULL);
	assert(po->count != 0);

	return po->func(po->data, b, pkt_size);
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

static 
struct oskit_netio_ops client_netio_ops = {
	net_query,
	net_addref, 
	net_release,
	net_push,
	net_alloc_bufio
};

oskit_netio_t *
oskit_netio_create_cleanup(oskit_error_t (*func)(void *data, oskit_bufio_t *b,
				   oskit_size_t pkt_size),
	      void *data, void (*destructor)(void *data))
{
	struct client_netio *c;

	c = (struct client_netio *)malloc(sizeof(*c));
        if (c == NULL)
                return NULL;

        c->ioi.ops = &client_netio_ops;
        c->count = 1;
	c->func = func;
	c->data = data;
	c->cleanup = destructor;

        return &c->ioi;
}

oskit_netio_t *
oskit_netio_create(oskit_error_t (*func)(void *data, oskit_bufio_t *b,
				   oskit_size_t pkt_size),
	      void *data)
{
	return oskit_netio_create_cleanup(func, data, NULL);
}

