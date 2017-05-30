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
#include <oskit/dev/osenv_rtc.h>

#include "native.h"

/*
 * There is one and only one interrupt interface in this implementation.
 */
static struct oskit_osenv_rtc_ops	osenv_rtc_ops;
static struct oskit_osenv_rtc		osenv_rtc_object={&osenv_rtc_ops};

static OSKIT_COMDECL
rtc_query(oskit_osenv_rtc_t *s, const oskit_iid_t *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_osenv_rtc_iid, sizeof(*iid)) == 0) {
                *out_ihandle = s;
                return 0;
        }

        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
rtc_addref(oskit_osenv_rtc_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_U
rtc_release(oskit_osenv_rtc_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL
rtc_get(oskit_osenv_rtc_t *o, struct oskit_timespec *time)
{
	return oskit_rtc_get(time);
}

static OSKIT_COMDECL_V
rtc_set(oskit_osenv_rtc_t *o, struct oskit_timespec *time)
{
	oskit_rtc_set(time);
}

static struct oskit_osenv_rtc_ops osenv_rtc_ops = {
	rtc_query,
	rtc_addref,
	rtc_release,
	rtc_get,
	rtc_set,
};

/*
 * Return a reference to the one and only interrupt object.
 */
oskit_osenv_rtc_t *
oskit_create_osenv_rtc(void)
{
	return &osenv_rtc_object;
}
