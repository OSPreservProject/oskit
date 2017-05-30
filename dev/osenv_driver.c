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
 * Default osenv driver registration object.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv_driver.h>

/*
 * There is one and only one driver/panic interface in this implementation.
 */
static struct oskit_osenv_driver_ops	osenv_driver_ops;
#ifdef KNIT
       struct oskit_osenv_driver	osenv_driver_object =
#else
static struct oskit_osenv_driver	osenv_driver_object =
#endif
						{&osenv_driver_ops};

static OSKIT_COMDECL
driver_query(oskit_osenv_driver_t *s, const oskit_iid_t *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_osenv_driver_iid, sizeof(*iid)) == 0) {
                *out_ihandle = s;
                return 0;
        }

        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
driver_addref(oskit_osenv_driver_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_U
driver_release(oskit_osenv_driver_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL
driver_register(oskit_osenv_driver_t *o, struct oskit_driver *driver,
	       const struct oskit_guid *iids, unsigned iid_count)
{
	return osenv_driver_register(driver, iids, iid_count);
}

static OSKIT_COMDECL
driver_unregister(oskit_osenv_driver_t *o, struct oskit_driver *driver,
	   const struct oskit_guid *iids, unsigned iid_count)
{
	return osenv_driver_unregister(driver, iids, iid_count);
}

static OSKIT_COMDECL
driver_lookup(oskit_osenv_driver_t *o, const struct oskit_guid *iid,
	      void ***out_interface_array)
{
	return osenv_driver_lookup(iid, out_interface_array);
}

static struct oskit_osenv_driver_ops osenv_driver_ops = {
	driver_query,
	driver_addref,
	driver_release,
	driver_register,
	driver_unregister,
	driver_lookup,
};

#ifndef KNIT
/*
 * Return a reference to the one and only interrupt object.
 */
oskit_osenv_driver_t *
oskit_create_osenv_driver(void)
{
	return &osenv_driver_object;
}
#endif
