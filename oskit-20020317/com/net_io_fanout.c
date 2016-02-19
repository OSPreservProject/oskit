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
 * netio_fanout object implementation.
 *
 * This provides a simple adaptor which can distribute packets to 
 * multiple recipients. It exports the netio interface, but provides
 * additional methods to add and remove listeners.
 */

#include <oskit/io/netio.h>
#include <oskit/io/netio_fanout.h>
#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/assert.h>
#include <oskit/c/malloc.h>

/*
 * keep listeners in a simple linked list
 */
struct netiolistener {
	oskit_netio_t		*netio;
	struct netiolistener 	*next;
};

/*
 * the object implementing the COM interface
 */
struct client_netio_fanout {
	oskit_netio_fanout_t	ioi;		/* COM I/O Interface */
	unsigned 		count;		/* reference count */
	struct netiolistener	*head;		/* list of listeners */
};
typedef struct client_netio_fanout client_netio_fanout_t;

/*
 * forward declarations
 */
static OSKIT_COMDECL 
	net_remove_listener(oskit_netio_fanout_t *ioi, oskit_netio_t *c);

/*
 * Query a netio_fanout I/O object for its interfaces.
 */
static OSKIT_COMDECL
net_query(oskit_netio_fanout_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
	struct client_netio_fanout *po = (struct client_netio_fanout*)io;

	assert(po != NULL);
	assert(po->count != 0);

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_netio_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_netio_fanout_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &po->ioi;
		++po->count;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}


/*
 * Clone a reference to a netio_fanout instance
 */
static OSKIT_COMDECL_U
net_addref(oskit_netio_fanout_t *io)
{
	struct client_netio_fanout *po = (struct client_netio_fanout*)io;

	assert(po != NULL);
	assert(po->count != 0);

	return ++po->count;
}


/*
 * Close ("release") a netio_fanout
 */
static OSKIT_COMDECL_U
net_release(oskit_netio_fanout_t *io)
{
	struct client_netio_fanout *po = (struct client_netio_fanout*)io;
	unsigned newcount;

	assert(po != NULL);
	assert(po->count != 0);

	/*
	 * note that since we're using net_remove_listener to get rid of
	 * listener if this is the last close, we cannot reset our reference
	 * count.
	 */
	newcount = po->count - 1;
	if (newcount == 0) {
		/* remove all listener if fanout object is destroyed */
		while (po->head)
			net_remove_listener(io, po->head->netio);

		free(po);
		return 0;
	}

	return po->count = newcount;
}


/*
 * Receive a packet and forward it on to all registered listeners
 *
 * return the last non-zero return code or zero
 */
static OSKIT_COMDECL
net_push(oskit_netio_fanout_t *io, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	struct client_netio_fanout *po = (struct client_netio_fanout*)io;
	struct netiolistener *l;
	oskit_error_t rc = 0, rc1;

	assert(po != NULL);
	assert(po->count != 0);

	for  (l = po->head; l; l = l->next) {
		rc1 = oskit_netio_push(l->netio, b, pkt_size);
		rc = rc1 ? rc1 : rc;
	}
	return rc;
}

/*
 * add a listener to the fanout
 */
static OSKIT_COMDECL    
net_add_listener(oskit_netio_fanout_t *io, oskit_netio_t *c)
{
	struct client_netio_fanout *po = (struct client_netio_fanout*)io;
	struct netiolistener *l;

	assert(po != NULL);
	assert(po->count != 0);

	l = malloc(sizeof *l);
	l->netio = c;
	oskit_netio_addref(c);
	l->next = po->head;
	po->head = l;
	return 0;
}

/*
 * remove a listener from a fanout
 */
static OSKIT_COMDECL
net_remove_listener(oskit_netio_fanout_t *io, oskit_netio_t *c)
{
	struct client_netio_fanout *po = (struct client_netio_fanout*)io;
	struct netiolistener **l, *ltemp;

	assert(po != NULL);
	assert(po->count != 0);

	for (l = &po->head; *l; l = &(*l)->next)
		if ((*l)->netio == c) {
			ltemp = (*l)->next;
			free(*l);
			*l = ltemp; 
			oskit_netio_release(c);	
			return 0;
		}
	return OSKIT_E_INVALIDARG;
}

/*
 * vtable for netio_fanout
 */
struct oskit_netio_fanout_ops client_netio_fanout_ops = {
	net_query,
	net_addref, 
	net_release,
	net_push,
	net_add_listener,
	net_remove_listener
};

/*
 * create a fanout object with no listeners
 */
oskit_netio_fanout_t *
oskit_netio_fanout_create()
{
	struct client_netio_fanout *c;

	c = (struct client_netio_fanout *)malloc(sizeof(*c));
        if (c == NULL)
                return NULL;

        c->ioi.ops = &client_netio_fanout_ops;
        c->count = 1;
	c->head = 0;

        return &c->ioi;
}
