/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
 * An adapter that turns an oskit_sleep_t into an oskit_listener_t.
 */

#include <oskit/com.h>
#include <oskit/com/listener.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv_sleep.h>
#include <oskit/dev/soa.h>

#include <malloc.h>
#include <string.h>

typedef struct listener_impl {
	oskit_listener_t listener;
        unsigned int refs;
	oskit_sleep_t *sleeper;
} listener_impl_t;

static OSKIT_COMDECL
listener_query(oskit_listener_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
        listener_impl_t *li = (listener_impl_t *)io;

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_listener_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &li->listener;
                ++li->refs;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
listener_addref(oskit_listener_t *io)
{
        listener_impl_t *li = (listener_impl_t *)io;
        return ++li->refs;
}

static OSKIT_COMDECL_U
listener_release(oskit_listener_t *io)
{
        listener_impl_t *li = (listener_impl_t *)io;
	if (--li->refs > 0)
		return li->refs;
	sfree(li, sizeof *li);
	return 0;
}

/* ARGSUSED */
static OSKIT_COMDECL
listener_notify(oskit_listener_t *io, oskit_iunknown_t *obj)
{
        listener_impl_t *li = (listener_impl_t *)io;
	oskit_sleep_wakeup(li->sleeper, OSKIT_SLEEP_WAKEUP);
	return 0;
}

static struct oskit_listener_ops oskit_listener_ops = {
	listener_query,
	listener_addref,
	listener_release,
	listener_notify
};

oskit_listener_t *
oskit_sleep_create_listener(oskit_sleep_t *sleeper)
{
	listener_impl_t	*li = smalloc(sizeof *li);

	if (li == 0)
		return 0;

	li->listener.ops = &oskit_listener_ops;
	li->refs = 1;
	li->sleeper = sleeper;

	return &li->listener;
}
