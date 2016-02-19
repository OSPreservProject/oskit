/*
 * Copyright (c) 1996, 1998, 1999, 2000 University of Utah and the Flux Group.
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
 * Default timer control object.
 */
#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv_timer.h>

#include "native.h"

/*
 * There is one and only one interrupt interface in this implementation.
 */
static struct oskit_osenv_timer_ops	osenv_timer_ops;
static struct oskit_osenv_timer		osenv_timer_object={&osenv_timer_ops};

static OSKIT_COMDECL
timer_query(oskit_osenv_timer_t *s, const oskit_iid_t *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_osenv_timer_iid, sizeof(*iid)) == 0) {
                *out_ihandle = s;
                return 0;
        }

        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
timer_addref(oskit_osenv_timer_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_U
timer_release(oskit_osenv_timer_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_V
timer_init(oskit_osenv_timer_t *o)
{
	osenv_timer_init();
}

static OSKIT_COMDECL_V
timer_shutdown(oskit_osenv_timer_t *o)
{
	osenv_timer_shutdown();
}

static OSKIT_COMDECL_V
timer_spin(oskit_osenv_timer_t *o, long nanosec)
{
	osenv_timer_spin(nanosec);
}

static OSKIT_COMDECL_V
timer_register(oskit_osenv_timer_t *o, void (*func)(void), int freq)
{
	osenv_timer_register(func, freq);
}

static OSKIT_COMDECL_V
timer_unregister(oskit_osenv_timer_t *o, void (*func)(void), int freq)
{
	osenv_timer_unregister(func, freq);
}

static struct oskit_osenv_timer_ops osenv_timer_ops = {
	timer_query,
	timer_addref,
	timer_release,
	timer_init,
	timer_shutdown,
	timer_spin,
	timer_register,
	timer_unregister,
};

/*
 * Return a reference to the one and only interrupt object.
 */
oskit_osenv_timer_t *
oskit_create_osenv_timer(void)
{
	return &osenv_timer_object;
}
