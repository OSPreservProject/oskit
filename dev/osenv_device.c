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
 * Default osenv device registration object.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv_device.h>

/*
 * There is one and only one device/panic interface in this implementation.
 */
static struct oskit_osenv_device_ops	osenv_device_ops;
#ifdef KNIT
       struct oskit_osenv_device	osenv_device_object =
#else
static struct oskit_osenv_device	osenv_device_object =
#endif
						{&osenv_device_ops};

static OSKIT_COMDECL
device_query(oskit_osenv_device_t *s, const oskit_iid_t *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_osenv_device_iid, sizeof(*iid)) == 0) {
                *out_ihandle = s;
                return 0;
        }

        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
device_addref(oskit_osenv_device_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_U
device_release(oskit_osenv_device_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL
device_register(oskit_osenv_device_t *o, struct oskit_device *device,
	       const struct oskit_guid *iids, unsigned iid_count)
{
	return osenv_device_register(device, iids, iid_count);
}

static OSKIT_COMDECL
device_unregister(oskit_osenv_device_t *o, struct oskit_device *device,
	   const struct oskit_guid *iids, unsigned iid_count)
{
	return osenv_device_unregister(device, iids, iid_count);
}

static OSKIT_COMDECL
device_lookup(oskit_osenv_device_t *o, const struct oskit_guid *iid,
	      void ***out_interface_array)
{
	return osenv_device_lookup(iid, out_interface_array);
}

static struct oskit_osenv_device_ops osenv_device_ops = {
	device_query,
	device_addref,
	device_release,
	device_register,
	device_unregister,
	device_lookup,
};

#ifndef KNIT
/*
 * Return a reference to the one and only interrupt object.
 */
oskit_osenv_device_t *
oskit_create_osenv_device(void)
{
	return &osenv_device_object;
}
#endif
