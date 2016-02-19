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
 * Default ioport object.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv_ioport.h>

/*
 * There is one and only one ioport interface in this implementation.
 */
static struct oskit_osenv_ioport_ops	osenv_ioport_ops;
#ifdef KNIT
       struct oskit_osenv_ioport	osenv_ioport_object =
#else
static struct oskit_osenv_ioport	osenv_ioport_object =
#endif
						  {&osenv_ioport_ops};

static OSKIT_COMDECL
ioport_query(oskit_osenv_ioport_t *s,
	     const oskit_iid_t *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_osenv_ioport_iid, sizeof(*iid)) == 0) {
                *out_ihandle = s;
                return 0;
        }

        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
ioport_addref(oskit_osenv_ioport_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_U
ioport_release(oskit_osenv_ioport_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_U
ioport_avail(oskit_osenv_ioport_t *o, oskit_addr_t port, oskit_size_t size)
{
	return osenv_io_avail(port, size);
}

static OSKIT_COMDECL
ioport_alloc(oskit_osenv_ioport_t *o, oskit_addr_t port, oskit_size_t size)
{
	return osenv_io_alloc(port, size);
}

static OSKIT_COMDECL_V
ioport_free(oskit_osenv_ioport_t *o, oskit_addr_t port, oskit_size_t size)
{
	return osenv_io_free(port, size);
}

static struct oskit_osenv_ioport_ops osenv_ioport_ops = {
	ioport_query,
	ioport_addref,
	ioport_release,
	ioport_avail,
	ioport_alloc,
	ioport_free,
};

#ifndef KNIT
/*
 * Return a reference to the one and only ioport interface object.
 */
oskit_osenv_ioport_t *
oskit_create_osenv_ioport(void)
{
	return &osenv_ioport_object;
}
#endif
