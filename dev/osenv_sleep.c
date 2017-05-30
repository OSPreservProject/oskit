/*
 * Copyright (c) 1996, 1998, 1999, 2002 University of Utah and the Flux Group.
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
 * Sleep/wakeup object. This implementation does very little!
 */
#include <oskit/debug.h>
#include <oskit/com.h>
#include <oskit/com/services.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv_sleep.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

/* COM object for oskit_osenv_sleep */
static struct oskit_osenv_sleep_ops	my_sleep_ops;
#ifdef KNIT
       struct oskit_osenv_sleep		my_sleep_object = { &my_sleep_ops };
#else
static struct oskit_osenv_sleep		my_sleep_object = { &my_sleep_ops };
#endif

/***************************************************************************
 *
 * Older osenv sleep interface. This includes a factory to create a
 * new-style sleep object (see below).
 */
static OSKIT_COMDECL
simple_sleep_query(oskit_osenv_sleep_t *b,
		  const struct oskit_guid *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_osenv_sleep_iid, sizeof(*iid)) == 0) {
		*out_ihandle = b;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
simple_sleep_addref(oskit_osenv_sleep_t *b)
{
	return 1;
}

static OSKIT_COMDECL_U
simple_sleep_release(oskit_osenv_sleep_t *b)
{
	return 1;
}

static OSKIT_COMDECL_V
simple_sleep_init(oskit_osenv_sleep_t *o, osenv_sleeprec_t *sr)
{
	osenv_sleep_init(sr);
}

static OSKIT_COMDECL_U
simple_sleep_sleep(oskit_osenv_sleep_t *o, osenv_sleeprec_t *sr)
{
	return osenv_sleep(sr);
}

static OSKIT_COMDECL_V
simple_sleep_wakeup(oskit_osenv_sleep_t *o,
		   osenv_sleeprec_t *sr, int wakeup_status)
{
	osenv_wakeup(sr, wakeup_status);
}

/*
 * function to create a sleep object.
 */
static OSKIT_COMDECL
simple_sleep_create(oskit_osenv_sleep_t *b, oskit_sleep_t **out_sleep)
{
	static oskit_error_t sleep_object_create(oskit_sleep_t **);

	return sleep_object_create(out_sleep);
}

static struct oskit_osenv_sleep_ops my_sleep_ops = {
	simple_sleep_query,
	simple_sleep_addref,
	simple_sleep_release,
	simple_sleep_init,
	simple_sleep_sleep,
	simple_sleep_wakeup,
	simple_sleep_create,
};

#ifndef KNIT
/*
 * Return a reference to the "default" sleep object factory.
 *
 * In this implementation, the oskit_osenv_intr_t parameter is ignored.
 * in favor of direct calls into other parts of the device library. A
 * different implementation is free to do something more complicated.
 */
oskit_osenv_sleep_t *
oskit_create_osenv_sleep(oskit_osenv_intr_t *iface)
{
	return &my_sleep_object;
}
#endif

/***************************************************************************
 *
 * New sleep object interface. Will eventually replace above interface.
 */

/* Ops for new sleep object */
static struct oskit_sleep_ops sleep_ops;

struct sleeper {
	oskit_sleep_t		sleepi;		/* Sleep COM interface */
	oskit_u32_t		count;		/* Reference count */
	osenv_sleeprec_t	sleeprec;	/* Sleep record */
};

static OSKIT_COMDECL
sleep_query(oskit_sleep_t *s, const oskit_iid_t *iid, void **out_ihandle)
{
	struct sleeper	*sleeper = (struct sleeper *) s;

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_sleep_iid, sizeof(*iid)) == 0) {
                *out_ihandle = s;
		sleeper->count++;
                return 0;
        }

        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
sleep_addref(oskit_sleep_t *s)
{
	/* I can't imagine it makes any sense to share a sleep object! */
	osenv_panic(__FILE__":sleep_addref: "
	      "Adding a reference to a sleep object? %p", s);

	return 0;
}

static OSKIT_COMDECL_U
sleep_release(oskit_sleep_t *s)
{
	struct sleeper	*sleeper = (struct sleeper *) s;
	int		newcount;

	osenv_assert(sleeper->count);

	if ((newcount = --sleeper->count) == 0) {
		free(sleeper);
		return 0;
	}

	return newcount;
}

static OSKIT_COMDECL_U
sleep_sleep(oskit_sleep_t *s)
{
	struct sleeper	*sleeper = (struct sleeper *) s;
	oskit_u32_t	status;


	status = osenv_sleep(&sleeper->sleeprec);

	/*
	 * The sleep object might be reused, so reinitialize it for
	 * next time.
	 */
	osenv_sleep_init(&sleeper->sleeprec);

	return status;
}

static OSKIT_COMDECL_U
sleep_sleep_consume(oskit_sleep_t *s)
{
	struct sleeper	*sleeper = (struct sleeper *) s;
	oskit_u32_t	status;


	status = osenv_sleep(&sleeper->sleeprec);

	/*
	 * The sleep object is used once and released.
	 */
	if (sleep_release(s))
		osenv_panic(__FILE__":sleep_consume: non-zero reference count");

	return status;
}

static OSKIT_COMDECL_V
sleep_wakeup(oskit_sleep_t *s, oskit_u32_t status)
{
	struct sleeper	*sleeper = (struct sleeper *) s;

	osenv_wakeup(&sleeper->sleeprec, status);
}

/*
 * Internal function to create a sleep object.
 */
static oskit_error_t
sleep_object_create(oskit_sleep_t **out_sleep)
{
	struct sleeper	*sleeper;

	if ((sleeper = (struct sleeper *) malloc(sizeof(*sleeper))) == NULL)
		return OSKIT_ENOMEM;

	sleeper->count = 1;
	sleeper->sleepi.ops = &sleep_ops;
	osenv_sleep_init(&sleeper->sleeprec);

	*out_sleep = &sleeper->sleepi;
	return 0;
}


static struct oskit_sleep_ops sleep_ops = {
	sleep_query,
	sleep_addref,
	sleep_release,
	sleep_sleep,
	sleep_sleep_consume,
	sleep_wakeup,
};
