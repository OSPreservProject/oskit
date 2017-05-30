/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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
 * Default log/panic object.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv_log.h>

/*
 * There is one and only one log/panic interface in this implementation.
 */
static struct oskit_osenv_log_ops	osenv_log_ops;
#ifdef KNIT
       struct oskit_osenv_log		osenv_log_object = {&osenv_log_ops};
#else
static struct oskit_osenv_log		osenv_log_object = {&osenv_log_ops};
#endif

static OSKIT_COMDECL
log_query(oskit_osenv_log_t *s, const oskit_iid_t *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_osenv_log_iid, sizeof(*iid)) == 0) {
                *out_ihandle = s;
                return 0;
        }

        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
log_addref(oskit_osenv_log_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_U
log_release(oskit_osenv_log_t *s)
{
	/* Only one object */
	return 1;
}


static OSKIT_COMDECL_V
log_vlog(oskit_osenv_log_t *o, int priority, const char *fmt, void *args)
{
	osenv_vlog(priority, fmt, args);
}

static OSKIT_COMDECL_V
log_log(oskit_osenv_log_t *o, int priority, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	osenv_vlog(priority, fmt, args);
	va_end(args);
}

static OSKIT_COMDECL_V
log_vpanic(oskit_osenv_log_t *o, const char *fmt, void *args)
{
	osenv_vpanic(fmt, args);
}

static OSKIT_COMDECL_V
log_panic(oskit_osenv_log_t *o, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	osenv_vpanic(fmt, args);
	va_end(args);
}

static struct oskit_osenv_log_ops osenv_log_ops = {
	log_query,
	log_addref,
	log_release,
	log_log,
	log_vlog,
	log_panic,
	log_vpanic,
};

#ifndef KNIT
/*
 * Return a reference to the one and only interrupt object.
 */
oskit_osenv_log_t *
oskit_create_osenv_log(void)
{
	return &osenv_log_object;
}
#endif
