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
 * Another netio example
 *
 * A simple unsafe packet filter which invokes a user-defined function
 * on any incoming packet. The function decides whether the packet
 * is to be pushed or not.
 */

#include <oskit/io/netio.h>
#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/malloc.h>
#include <oskit/c/assert.h>

struct uspf_netio_impl {
	oskit_netio_t	ioi;		/* COM I/O Interface */
	unsigned 	count;		/* reference count */

	/* return 1 if packet if accepted */
	int		(*func)(void *frame, oskit_size_t pktsize);
	oskit_netio_t 	*forward_to;
};
typedef struct uspf_netio_impl uspf_netio_impl_t;

/*
 * Query a net I/O object for its interfaces.
 */
static OSKIT_COMDECL
net_query(oskit_netio_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
	struct uspf_netio_impl *po = (struct uspf_netio_impl *)io;

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
 * Clone a reference to the netio interface
 */
static OSKIT_COMDECL_U
net_addref(oskit_netio_t *io)
{
	struct uspf_netio_impl *po = (struct uspf_netio_impl *)io;

	assert (po != NULL);
	assert (po->count != 0);

	return ++po->count;
}


/*
 * Close ("release") a device.
 */
static OSKIT_COMDECL_U
net_release(oskit_netio_t *io)
{
	struct uspf_netio_impl *po = (struct uspf_netio_impl *)io;
	unsigned newcount;

	assert (po != NULL);
	assert (po->count != 0);

	newcount = --po->count;
	if (newcount == 0) {
		free(po);
	}

	return newcount;
}


/*
 * Receive a packet and forward it on to the callback function.
 */
static OSKIT_COMDECL
net_push(oskit_netio_t *io, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	struct uspf_netio_impl *po = (struct uspf_netio_impl *)io;
	static unsigned char *frame;
	oskit_error_t err, rc = 0;

	assert (po != NULL);
	assert (po->count != 0);

        /* check to see if the bufio matches the filter */
        err = oskit_bufio_map(b, (void **)&frame, 0, pkt_size);

	/* XXX read in if map fails */
	if (err)
		return err;

	/* check callback function */
	if (po->func(frame, pkt_size)) {
                /* matches, call the netio's push routine */
                rc = oskit_netio_push(po->forward_to, b, pkt_size);
        } 

        err = oskit_bufio_unmap(b, (void *)frame, 0, pkt_size);
        assert(!err);
	return rc;
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
 * vtable
 */
static struct oskit_netio_ops uspf_netio_impl_ops = {
	net_query,
	net_addref, 
	net_release,
	net_push,
	net_alloc_bufio
};


/*
 * create a unsafe packet filter
 */
oskit_netio_t *
oskit_netio_uspf_create(int (*func)(void *data, oskit_size_t pkt_size), 
	oskit_netio_t *push_on)
{
	struct uspf_netio_impl *c;

	c = (struct uspf_netio_impl *)malloc(sizeof(*c));
        if (c == NULL)
                return NULL;

        c->ioi.ops = &uspf_netio_impl_ops;
        c->count = 1;
	c->func = func;
	c->forward_to = push_on;

        return &c->ioi;
}

