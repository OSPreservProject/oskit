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

/* A simple adaptor to implement a callback using oskit_listener's. */

#include <oskit/com.h>
#include <oskit/com/listener.h>
#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>

typedef struct listener_impl {
	oskit_listener_t			iol;	/* COM interface */
        unsigned                	count;  /* reference count */
	oskit_listener_callback_t	cb;	/* callback */
	void				*arg;	/* and its argument */
} listener_impl_t;

static OSKIT_COMDECL
listener_query(oskit_listener_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
        listener_impl_t *li = (listener_impl_t *)io;

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_listener_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &li->iol;
                ++li->count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
listener_addref(oskit_listener_t *io)
{
        listener_impl_t *li = (listener_impl_t *)io;

        if (li->count == 0)
		return OSKIT_E_INVALIDARG;

        return ++li->count;
}

static OSKIT_COMDECL_U
listener_release(oskit_listener_t *io)
{
        listener_impl_t *li = (listener_impl_t *)io;
        unsigned newcount;

        if (li == NULL || li->count == 0)
		return OSKIT_E_INVALIDARG;

        newcount = --li->count;
        if (newcount == 0) {
		free(li);
        }

        return newcount;
}

/* ARGSUSED */
static OSKIT_COMDECL
listener_notify(oskit_listener_t *io, oskit_iunknown_t *obj)
{
        listener_impl_t *li = (listener_impl_t *)io;
	return li->cb(obj, li->arg);
}

static struct oskit_listener_ops oskit_listener_ops = {
	listener_query,
	listener_addref,
	listener_release,
	listener_notify
};

oskit_listener_t *
oskit_create_listener(oskit_listener_callback_t handler, void *arg)
{
	listener_impl_t	*li = malloc(sizeof *li);

	if (li == 0)
		return 0;

	memset(li, 0, sizeof *li);
	li->cb = handler;
	li->arg = arg;
	li->iol.ops = &oskit_listener_ops;
	li->count = 1;

	return &li->iol;
}
