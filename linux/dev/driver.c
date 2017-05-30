/*
 * Copyright (c) 1996-1999 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

#include <oskit/com.h>
#include <oskit/dev/isa.h>
#include <oskit/dev/native.h>	/* XXX: osenv_isabus_init */
#include <linux/string.h>
#include <linux/sched.h>

#include "glue.h"
#include "sched.h"
#include "osenv.h"

oskit_error_t
oskit_linux_driver_register(struct driver_struct *drv)
{
	oskit_error_t rc;

	/* First make sure the generic Linux driver code is initialized */
	rc = oskit_linux_dev_init();
	if (rc)
		return rc;

#ifndef KNIT
	/* Also, make sure the ISA bus tree is initialized. */
	osenv_isabus_init();
#endif

	/* Register this driver */
	rc = osenv_driver_register((oskit_driver_t*)&drv->drvi,
				  &oskit_isa_driver_iid, 1);
	if (rc)
		return rc;

	return 0;
}

static OSKIT_COMDECL
query(oskit_isa_driver_t *drv, const struct oskit_guid *iid, void **out_ihandle)
{
	struct driver_struct *ds = (struct driver_struct*)drv;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_driver_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_isa_driver_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &ds->drvi;
		return 0;
	}

	*out_ihandle = 0;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U addref(oskit_isa_driver_t *drv)
{
	/* No reference counting for static driver nodes */
	return 1;
}

static OSKIT_COMDECL_U release(oskit_isa_driver_t *drv)
{
	/* No reference counting for static driver nodes */
	return 1;
}

static OSKIT_COMDECL getinfo(oskit_isa_driver_t *drv, oskit_devinfo_t *out_info)
{
	struct driver_struct *ds = (struct driver_struct*)drv;

	*out_info = ds->info;
	return 0;
}

static OSKIT_COMDECL probe(oskit_isa_driver_t *drv, oskit_isabus_t *bus)
{
	struct driver_struct *ds = (struct driver_struct*)drv;
	struct task_struct ts;
	oskit_error_t rc;

	OSKIT_LINUX_CREATE_CURRENT(ts);

	/* Now probe for the specific device(s) we're looking for */
	rc = ds->probe(ds);

	OSKIT_LINUX_DESTROY_CURRENT();

	return rc;
}

struct oskit_isa_driver_ops oskit_linux_driver_ops = {
	query, addref, release,
	getinfo, probe
};

